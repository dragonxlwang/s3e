//////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Xiaolong Wang All Rights Reserved.        //
// Code for ``Dual Clustered Maximal Entropy for Many Classes'' //
//////////////////////////////////////////////////////////////////

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"
#include "variables.c"

// model parameter is shared across all threads
struct Model {
  real* scr;  // source vector;
  real* tar;  // target vector;
} * model;

// each thread worker maintains a bookkeeping
struct Bookkeeping {
  real* dd;   // list(K):vector(V) dual distribution
  real* ent;  // list(K):
  real* ww;   // list(K):vector(N) W * dd, synthesis word in prime
  real* hh;   // list(K):vector(N) avg(h_k), sufficient stats
  int* hn;    // list(K): |h_k|
#ifdef DEBUG
  real acc_sum;
  int acc_cnt;
#endif
};

real gd_ss;  // gradient descent step size
int max_hn;  // when hn exceeds max_hn, performs offline update for one dist

// current progress for each worker
real* progress;

// vocabulary
struct Vocabulary* vcb;

// start time
clock_t start_clock_t;

// some helper functions for debugging
real GetProgress() {
  real p = NumSumVec(progress, V_THREAD_NUM);
  p /= (V_THREAD_NUM * V_ITER_NUM);
  return p;
}

int preceed_newline_flag = 1;
int ppb_lock = 0;
#ifdef DEBUG
real acum_acc = -1;
#endif
void ThreadPrintProgBar(int dbg_lvl, int tid, real p, struct Bookkeeping* b) {
  if (ppb_lock) return;
  ppb_lock = 1;
  if (preceed_newline_flag) {
    LOG(dbg_lvl, "\n");
    preceed_newline_flag = 0;
  }
  // print progress
  int i, bar_len = 10;
  clock_t cur_clock_t = clock();
  real pct = p * 100;
  int bar = p * bar_len;
#ifdef DEBUG
  real ww = NumVecNorm(b->ww, K * N);
  real scr = NumVecNorm(model->scr, V * N);
  real ss = NumMatMaxRowNorm(model->scr, V, N);
  real tar = NumVecNorm(model->tar, V * N);
  real tt = NumMatMaxRowNorm(model->tar, V, N);
  real eem = NumVecMean(b->ent, K);
  real ees = NumVecStd(b->ent, K);
/* heap* h = HeapCreate(5); */
/* for (i = 0; i < V; i++) HeapPush(h, i, NumVecNorm(model->tar + i * N, N)); */
/* HeapSort(h); */
#endif
  /* LOGC(dbg_lvl, 'y', 'k', "\33[2K\r[%7.4lf%%]: ", pct); */
  LOGC(dbg_lvl, 'y', 'k', "\n[%7.4lf%%]: ", pct);
  for (i = 0; i < bar; i++) LOGC(dbg_lvl, 'r', 'k', "+");
  LOGC(dbg_lvl, 'r', 'k', "~");
  for (i = bar + 1; i < bar_len; i++) LOGC(dbg_lvl, 'c', 'k', "=");
  LOGC(dbg_lvl, 'g', 'k', " (tid=%02d)", tid);
  double elapsed_time = (double)(cur_clock_t - start_clock_t) / CLOCKS_PER_SEC;
  char* ht = strtime(elapsed_time / V_THREAD_NUM);
  LOG(dbg_lvl, " TIME:%.2e/", elapsed_time);
  LOGC(dbg_lvl, 'c', 'k', "%s", ht);
  LOG(dbg_lvl, " GDSS:%.4e", gd_ss);
  free(ht);
#ifdef DEBUG
  /* LOG(0, " ww=%.2e/", ww); */
  /* LOGC(0, 'y', 'k', "%.2ett ", ww / tt); */
  LOG(0, "scr=%.2e/%.2e ", scr, scr / ss);
  LOG(0, "tar=%.2e/", tar);
  LOGC(0, 'y', 'k', "%.2ett ", tar / tt);
  /* LOGC(0, 'c', 'k', "tt=%.2e ", tt); */
  /* for (i = 0; i < 5; i++) { */
  /*   LOGC(0, 'b', 'k', "%s", vcb->id2wd[h->d[i].key]); */
  /*   LOG(0, ":%lf ", h->d[i].val); */
  /* } */
  if (acum_acc < 0)
    acum_acc = b->acc_sum / b->acc_cnt;
  else
    acum_acc = 0.9 * acum_acc + 0.1 * b->acc_sum / b->acc_cnt;
  LOGC(0, 'r', 'c', " ACC:%.2e/%.2e", b->acc_sum / b->acc_cnt, acum_acc);
  LOGC(0, 'w', 'r', " ENT:%.2e/%.2e", eem, ees);
  b->acc_sum = 0;
  b->acc_cnt = 0;
#endif
  ppb_lock = 0;
  return;
}

struct Bookkeeping* BookkeepingCreate() {
  struct Bookkeeping* b =
      (struct Bookkeeping*)malloc(sizeof(struct Bookkeeping));
  b->dd = NumNewHugeVec(V * K);
  b->ent = NumNewHugeVec(K);
  b->ww = NumNewHugeVec(N * K);
  b->hh = NumNewHugeVec(N * K);
  b->hn = NumNewHugeIntVec(K);
  NumFillZeroVec(b->hh, N * K);
  NumFillZeroIntVec(b->hn, K);
  NumFillZeroVec(b->ent, K);
  NumRandFillVec(b->ww, N * K, -1e-1, 1e-1);
  NumFillValVec(b->dd, V * K, 1.0 / V);
#ifdef DEBUG
  b->acc_sum = 0;
  b->acc_cnt = 0;
#endif
  return b;
}

void ModelInit() {
  model = (struct Model*)malloc(sizeof(struct Model));
  model->scr = NumNewHugeVec(V * N);
  model->tar = NumNewHugeVec(V * N);
  NumRandFillVec(model->scr, V * N, -1e-1, 1e-1);
  NumRandFillVec(model->tar, V * N, -1e-1, 1e-1);
  return;
}

int saved_iter_num = -1;
void ModelSave(int iter_num) {
  char* mfp;
  if (iter_num == -1) {                 // save final model
    STR_CLONE(mfp, V_MODEL_SAVE_PATH);  // so that free(mfp) can work
  } else {
    if (iter_num <= saved_iter_num) return;  // avoid thread racing
    saved_iter_num = iter_num;
    mfp = sformat("%s.part%d", V_MODEL_SAVE_PATH, iter_num);
  }
  FILE* fout = fopen(mfp, "wb");
  if (!fout) {
    LOG(0, "Error!\n");
    exit(1);
  }
  fwrite(model->scr, sizeof(real), V * N, fout);
  fwrite(model->tar, sizeof(real), V * N, fout);
  fclose(fout);
  free(mfp);
  return;
}

void BookkeepingFree(struct Bookkeeping* b) {
  free(b->dd);
  free(b->ent);
  free(b->ww);
  free(b->hh);
  free(b->hn);
  free(b);
  return;
}

void ModelFree() {
  free(model->scr);
  free(model->tar);
  free(model);
  return;
}

void ModelGradUpdate(struct Model* m, int p, int i, real c, real* g) {
  // model minimization update: p=0: scr; p=1: tar
  if (p == 0) {  // update scr
    NumVecAddCVec(m->scr + i * N, g, -c * gd_ss, N);
  } else {  // update tar
    NumVecAddCVec(m->tar + i * N, g, -c * gd_ss, N);
  }
  return;
}

void ModelShrink(struct Model* m) {
  int i;
  if (V_L2_REGULARIZATION_WEIGHT != 0) {
    NumVecMulC(model->scr, 1 - V_L2_REGULARIZATION_WEIGHT, V * N);
    NumVecMulC(model->tar, 1 - V_L2_REGULARIZATION_WEIGHT, V * N);
  } else if (V_MODEL_PROJ_UNIT_BALL) {
    for (i = 0; i < V; i++) {
      NumVecProjUnitBall(m->scr + i * N, N);
      NumVecProjUnitBall(m->tar + i * N, N);
    }
  }
  return;
}

int DualDecode(real* h, struct Bookkeeping* b) {
  // fit the sentence with the best dual distribution (z)
  real s = 0, t;
  int z = 0, k;
  for (k = 0; k < K; k++) {
    t = NumVecDot(h, b->ww + k * N, N) + b->ent[k];
    if (k == 0 || t > s) {
      s = t;
      z = k;
    }
  }
  return z;
}

int Update(int* ids, int l, struct Bookkeeping* b, struct Model* m,
           int primal) {
  // update for one sentence (ids, l)
  // primal = 0: used for burn-in
  // primal = 1: primal and dual update
  int offline_performed = 0;
  int i, j, lt, rt, md;
  real h0[NUP], h[NUP], hw[SUP], wd[NUP * SUP], w0[NUP], w[NUP];
  int zz;
  NumFillZeroVec(h0, N);
  NumFillZeroVec(w0, N);
  for (i = 0; i < SMALLER(l, C); i++)
    NumVecAddCVec(h0, m->scr + ids[i] * N, 1, N);
  for (i = 0; i < l; i++) {
    lt = i - C - 1;
    rt = i + C;
    md = i;
    if (lt >= 0) NumVecAddCVec(h0, m->scr + ids[lt] * N, -1, N);      // h0--
    if (rt < l) NumVecAddCVec(h0, m->scr + ids[rt] * N, 1, N);        // h0++
    hw[md] = 1.0 / (SMALLER(rt, l - 1) - LARGER(lt, 0));              // hw
    NumAddCVecDVec(h0, m->scr + ids[md] * N, hw[md], -hw[md], N, h);  // h
    zz = DualDecode(h, b);                                            // dcd z
    // update bookkeeping: hh, hn
    NumVecAddCVec(b->hh + zz * N, h, 1.0, N);
    b->hn[zz]++;
#ifdef DEBUG
    b->acc_sum += b->dd[zz * V + ids[md]];  // debug probability
    b->acc_cnt++;
#endif
    if (primal) {                          // primal 1
      NumAddCVecDVec(m->tar + ids[i] * N,  // wd[i] <- w - ww
                     b->ww + zz * N, 1, -1, N, wd + i * N);
      ModelGradUpdate(m, 1, ids[i], -1, h);  // up m->tar
      lt = i - 2 * C - 1;
      rt = i;
      md = i - C;
      NumVecAddCVec(w0, wd + rt * N, hw[rt], N);  // w0++
      while (md < l) {
        if (lt >= 0) NumVecAddCVec(w0, wd + lt * N, -hw[lt], N);  // w0--
        if (md >= 0) {
          NumAddCVecDVec(w0, wd + md * N, 1, -hw[md], N, w);  // w
          ModelGradUpdate(m, 0, ids[md], -1, w);              // up m->scr
        }
        if (i == l - 1) {
          md++;
          lt++;
        } else
          break;
      }
    }
    if (b->hn[zz] >= max_hn) {  // offline
      offline_performed = 1;
      if (primal) {              // primal 2
        for (j = 0; j < V; j++)  // up m->tar (neg sampling)
          ModelGradUpdate(m, 1, j, b->dd[zz * V + j], b->hh + zz * N);
        ModelShrink(m);
      }
      NumVecMulC(b->hh + zz * N, 1.0 / b->hn[zz], N);              // hh
      NumMulMatVec(m->tar, b->hh + zz * N, V, N, b->dd + zz * V);  // dd
      b->ent[zz] = NumSoftMax(b->dd + zz * V, V);                  // ent
      NumMulVecMat(b->dd + zz * V, m->tar, V, N, b->ww + zz * N);  // ww
      b->hn[zz] = 0;                                               // reset hn
      NumFillZeroVec(b->hh + zz * N, N);                           // reset hh
    }
  }
  return offline_performed;
}

void* ThreadWork(void* arg) {
  int tid = (long)arg;
  FILE* fin = fopen(V_TEXT_FILE_PATH, "rb");
  if (!fin) {
    LOG(0, "Error!\n");
    exit(1);
  }
  fseek(fin, 0, SEEK_END);
  long int fsz = ftell(fin);
  long int fbeg = fsz / V_THREAD_NUM * tid;
  long int fend = fsz / V_THREAD_NUM * (tid + 1);
  long int fpos = fbeg;
  struct Bookkeeping* b = BookkeepingCreate();
  LOG(2, "*");
  // burn in
  int wids[SUP], wnum;
  long long int burn_in_left = V * V_BURN_IN_INTERVAL_VOCAB_RATIO;
  fseek(fin, fbeg, SEEK_SET);
  while (burn_in_left > 0) {
    wnum = TextReadSent(fin, vcb, wids, 1, 1);
    burn_in_left -= wnum;
    if (wnum > 1)
      if (Update(wids, wnum, b, model, 0)) {
        // nop
      }
    // ftell costs running time roughly 15/14 times of an addition operation
    if (feof(fin) || ftell(fin) >= fend) {
      fseek(fin, fbeg, SEEK_SET);
    }
  }
  if (preceed_newline_flag) LOG(2, "$");
  // training
  real p = 0;
  int iter_num = 0;
  fseek(fin, fbeg, SEEK_SET);
  while (iter_num < V_ITER_NUM) {
    wnum = TextReadSent(fin, vcb, wids, 1, 1);
    if (wnum > 1)
      if (Update(wids, wnum, b, model, 1)) {
        // thread progress
        progress[tid] = iter_num + (double)(fpos - fbeg) / (fend - fbeg);
        // gd_ss
        p = GetProgress();
        gd_ss = V_INIT_GRAD_DESCENT_STEP_SIZE * (1 - p);
        // debug info
        ThreadPrintProgBar(2, tid, p, b);
      }
    fpos = ftell(fin);
    if (feof(fin) || fpos >= fend) {
      fseek(fin, fbeg, SEEK_SET);
      if (V_CACHE_INTERMEDIATE_MODEL) ModelSave(iter_num);
      iter_num++;
    }
  }
  fclose(fin);
  BookkeepingFree(b);
  pthread_exit(NULL);
  return 0;
}

void ScheduleWork() {
  long long int tid;
  start_clock_t = clock();
  progress = (real*)malloc(V_THREAD_NUM * sizeof(real));
  NumFillZeroVec(progress, V_THREAD_NUM);
  // included initialization
  VariableInit();
  NumInit();
  // build vocab if necessary
  if (!fexists(V_VOCAB_FILE_PATH) || V_VOCAB_OVERWRITE) {
    vcb = TextBuildVocab(V_TEXT_FILE_PATH, 1, -1);
    TextSaveVocab(V_VOCAB_FILE_PATH, vcb);
  }
  // load vocab and overwrite V by smaller actual vocabulary size
  vcb = TextLoadVocab(V_VOCAB_FILE_PATH, V, V_VOCAB_HIGH_FREQ_CUTOFF);
  V = vcb->size;
  LOG(1, "Actual V: %d\n", V);
  // initialization
  ModelInit();
  max_hn = V * V_OFFLINE_INTERVAL_VOCAB_RATIO;
  // schedule thread jobs
  LOG(2, "Threads spawning: ");
  pthread_t* pt = (pthread_t*)malloc(V_THREAD_NUM * sizeof(pthread_t));
  for (tid = 0; tid < V_THREAD_NUM; tid++)
    pthread_create(&pt[tid], NULL, ThreadWork, (void*)tid);
  for (tid = 0; tid < V_THREAD_NUM; tid++) pthread_join(pt[tid], NULL);
  // save model
  ModelSave(-1);
  // free memory
  ModelFree();
  VocabDestroy(vcb);
  free(progress);
  free(pt);
  LOG(1, "\nTraining finished\n");
}

int main(int argc, char* argv[]) {
  ScheduleWork();
  return 0;
}

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
#include "peek.c"
#include "variables.c"

struct Vocabulary* vcb;
int** lwids;
int* lwnum;
long long int num;
int cap = 0xFFFFF;
void permute() {
  int i, j, k;
  int wids[SUP], wnum;
  // permute sentences in text
  VariableInit();
  NumInit();
  // build vocab if necessary, load, and set V by smaller actual size
  if (!fexists(V_VOCAB_FILE_PATH) || V_VOCAB_OVERWRITE) {
    vcb = TextBuildVocab(V_TEXT_FILE_PATH, 1, -1);
    TextSaveVocab(V_VOCAB_FILE_PATH, vcb);
  }
  vcb = TextLoadVocab(V_VOCAB_FILE_PATH, V, V_VOCAB_HIGH_FREQ_CUTOFF);
  V = vcb->size;
  LOG(1, "Actual V: %d\n", V);
  FILE* fin = fopen(V_TEXT_FILE_PATH, "rb");
  if (!fin) {
    LOG(0, "Error!\n");
    exit(1);
  }
  lwids = (int**)malloc(cap * sizeof(int*));
  lwnum = (int*)malloc(cap * sizeof(int));
  while (!feof(fin)) {
    wnum = TextReadSent(fin, vcb, wids, 1, 1);
    ARR_CLONE(lwids[num], wids, wnum);
    lwnum[num] = wnum;
    num++;
    if (num == cap) {
      cap *= 2;
      lwids = (int**)realloc(lwids, cap * sizeof(int*));
      lwnum = (int*)realloc(lwnum, cap * sizeof(int));
    }
  }
  int* idx = NumNewHugeIntVec(num);
  range(idx, num);
  NumPermuteIntVec(idx, num, 10.0);
  printf("reading %lld sentences\n", num);
  fclose(fin);
  FILE* fout = fopen(sformat("%s.perm", V_TEXT_FILE_PATH), "wb");
  for (i = 0; i < num; i++) {
    j = idx[i];
    for (k = 0; k < lwnum[j]; k++)
      fprintf(fout, "%s ", VocabGetWord(vcb, lwids[j][k]));
  }
  fclose(fout);
  return;
}

int main() { permute(); }
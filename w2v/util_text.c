#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util_misc.c"

////////////////
// Vocabulary //
////////////////

int HASH_SLOTS = 0xFFFFF;  // around 1M slots by default

#define STR_CLONE(d, s)                                \
  ({                                                   \
    d = (char*)malloc((strlen(s) + 1) * sizeof(char)); \
    strcpy(d, s);                                      \
  })

// BKDR Hash for string
int GetStrHash(char* str) {
  unsigned long long h = 0;
  char ch;
  while ((ch = *(str++))) h = (((h << 7) + (h << 1) + (h) + ch) & HASH_SLOTS);
  return h;
}

// vocabulary, a hash map
struct Vocabulary {
  char** id2wd;
  int* id2cnt;
  int* id2next;
  int* hash2head;
  int size;
  // initial settings
  int VCB_CAP;
};

void VocabSetHashSlots(int hs) { HASH_SLOTS = hs; }

void VocabInitStorage(struct Vocabulary* vcb, int cap) {
  vcb->id2wd = (char**)malloc(cap * sizeof(char*));
  vcb->id2cnt = (int*)malloc(cap * sizeof(int));
  vcb->id2next = (int*)malloc(cap * sizeof(int));
  vcb->hash2head = (int*)malloc(HASH_SLOTS * sizeof(int));
  if (!vcb->id2wd || !vcb->id2cnt || !vcb->id2next || !vcb->hash2head) {
    LOG(0, "allocating error @ Initvcb");
    exit(1);
  }
  vcb->size = 0;
  vcb->VCB_CAP = cap;
  memset(vcb->hash2head, 0xFF, HASH_SLOTS * sizeof(int));
}

void VocabClearStorage(struct Vocabulary* vcb) {
  int i;
  for (i = 0; i < vcb->size; i++) free(vcb->id2wd[i]);
  free(vcb->id2wd);
  free(vcb->id2cnt);
  free(vcb->id2next);
  free(vcb->hash2head);
  return;
}

struct Vocabulary* VocabCreate(int cap) {
  struct Vocabulary* vcb =
      (struct Vocabulary*)malloc(sizeof(struct Vocabulary));
  VocabInitStorage(vcb, cap);
  return vcb;
}

void VocabDestroy(struct Vocabulary* vcb) {
  VocabClearStorage(vcb);
  free(vcb);
  return;
}

void VocabAdd(struct Vocabulary* vcb, char* str, int cnt) {
  int h = GetStrHash(str);
  int id = vcb->hash2head[h];
  while (id != -1 && strcmp(vcb->id2wd[id], str) != 0) id = vcb->id2next[id];
  if (id == -1) {
    id = vcb->size++;
    STR_CLONE(vcb->id2wd[id], str);
    vcb->id2cnt[id] = 0;
    vcb->id2next[id] = vcb->hash2head[h];
    vcb->hash2head[h] = id;
    if (vcb->size + 2 > vcb->VCB_CAP) {
      vcb->VCB_CAP <<= 1;
      vcb->id2wd = (char**)realloc(vcb->id2wd, vcb->VCB_CAP * sizeof(char*));
      vcb->id2cnt = (int*)realloc(vcb->id2cnt, vcb->VCB_CAP * sizeof(int));
      vcb->id2next = (int*)realloc(vcb->id2next, vcb->VCB_CAP * sizeof(int));
    }
  }
  vcb->id2cnt[id] += cnt;
  return;
}

int* value_for_compare;
int CmpVal(const void* x, const void* y) {
  return value_for_compare[*((int*)y)] - value_for_compare[*((int*)x)];
}

void VocabReduce(struct Vocabulary* vcb, int cap) {
  int i;
  int* keys = (int*)malloc(vcb->size * sizeof(int));
  for (i = 0; i < vcb->size; i++) keys[i] = i;
  value_for_compare = vcb->id2cnt;
  qsort(keys, vcb->size, sizeof(int), CmpVal);
  int size = ((vcb->size > cap) ? cap : vcb->size);
  char** words = (char**)malloc(size * sizeof(char*));
  int* counts = (int*)malloc(size * sizeof(int));
  for (i = 0; i < size; i++) {
    STR_CLONE(words[i], vcb->id2wd[keys[i]]);
    counts[i] = vcb->id2cnt[keys[i]];
  }
  VocabClearStorage(vcb);
  VocabInitStorage(vcb, (int)(size * 1.1));
  for (i = 0; i < size; i++) VocabAdd(vcb, words[i], counts[i]);
  for (i = 0; i < size; i++) free(words[i]);
  free(keys);
  free(words);
  free(counts);
  return;
}

int VocabGet(struct Vocabulary* vcb, char* str) {
  int h = GetStrHash(str);
  int id = vcb->hash2head[h];
  while (id != -1 && strcmp(vcb->id2wd[id], str) != 0) id = vcb->id2next[id];
  return id;  // UNK = -1 or in vocab
}

//////////
// Text //
//////////

int text_init_vcb_cap = 0x200000;         // around 2M slots by default
int text_max_word_len = 100;              // one word has max 100 char
int text_max_sent_wct = 200;              // one sentence has max 200 word
long long int text_corpus_word_cnt = 0;   // corpus wourd count
long long int text_corpus_file_size = 0;  // corpus file size

/**
*  @return  flag : 0 hit by space or tab
*                  1 hit by newline
*                  2 hit by end-of-file
*/
int TextReadWord(FILE* fin, char* str) {
  int i = 0, flag = -1;
  char ch;
  while (1) {
    ch = fgetc(fin);
    if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == EOF) break;
    if (i < text_max_word_len - 1) str[i++] = ch;
  }
  str[i] = '\0';
  while (1) {
    if (ch == ' ' || ch == '\t')
      flag = 0;
    else if (ch == '\n' || ch == '\r') {
      flag = 1;
      if (fin == stdin) break;
    } else if (ch == EOF) {
      flag = 2;
      break;
    } else {
      ungetc(ch, fin);
      break;
    }
    ch = fgetc(fin);
  }
  return flag;
}

/**
 * @return  : if if_rm_trail_punc set 1, return 1 if str ends with .,:;?!
 */
int TextNormWord(char* str, int if_lower, int if_rm_trail_punc) {
  int i = 0, j, flag = 0;
  char ch;
  if (if_lower)
    while (str[i] != '\0') {
      if (str[i] >= 'A' && str[i] <= 'Z') str[i] = str[i] - 'A' + 'a';
      i++;
    }
  i--;
  if (if_rm_trail_punc) {
    while (i >= 0) {
      ch = str[i];
      if (ch == '.' || ch == ',' || ch == ':' || ch == ';' || ch == '?' ||
          ch == '!') {
        str[i] = '\0';  // removing clause-terminating punctuation
        flag = 1;
      } else if (ch == '`' || ch == '"' || ch == '\'' || ch == ')' ||
                 ch == ']') {
        str[i] = '\0';  // removing proceeding enclosing punctuation
      } else
        break;
      i--;
    }
    i = 0;
    while (1) {  // removing preceding enclosing punctuation
      ch = str[i];
      if (ch == '`' || ch == '"' || ch == '\'' || ch == '(' || ch == '[')
        i++;
      else
        break;
    }
    if (i != 0)
      for (j = i; str[j - 1] != '\0'; j++) str[j - i] = str[j];
  }
  return flag;
}

void TextBuildVocab(char* text_file_path, int if_norm_word) {
  char* str = (char*)malloc(text_max_word_len);
  int flag = 0;
  struct Vocabulary* vcb = VocabCreate(text_init_vcb_cap);
  FILE* fin = fopen(text_file_path, "rb");
  text_corpus_word_cnt = 0;
  if (!fin) {
    LOG(0, "[error]: cannot find file %s\n", text_file_path);
    exit(1);
  }
  while (1) {
    flag = TextReadWord(fin, str);
    if (if_norm_word) TextNormWord(str, 1, 1);
    if (str[0] != '\0') {  // filter 0-length string
      VocabAdd(vcb, str, 1);
      text_corpus_word_cnt++;
      if ((text_corpus_word_cnt & 0xFFFFF) == 0xFFFFF)
        LOG(2, "[TextBuildVocab]: reading %lld [*2^20 | M] word\r",
            text_corpus_word_cnt >> 20);
    }
    if (flag == 2) break;
  }
  LOG(1, "[TextBuildVocab]: reading %lld [*2^20 | M] word\r",
      text_corpus_word_cnt >> 20);
  text_corpus_file_size = ftell(fin);
  LOG(1, "[TextBuildVocab]: file size %lld [*2^20 | M]\n",
      text_corpus_file_size >> 20);
  fclose(fin);
  return;
}

int TextReadSent(FILE* fin, struct Vocabulary* vcb, int* word_ids,
                 int if_norm_word) {
  char* str = (char*)malloc(text_max_word_len);
  int flag1, flag2 = 0, id;
  int word_num = 0;
  while (1) {
    flag1 = TextReadWord(fin, str);
    if (if_norm_word) flag2 = TextNormWord(str, 1, 1);
    if (str[0] == '\0')
      id = -1;  // empty string
    else
      id = VocabGet(vcb, str);  // non-empty string
    if (id != -1 && word_num < text_max_sent_wct) word_ids[word_num++] = id;
    if (flag1 > 0 || flag2 == 1) break;
  }
  return word_num;
}

void SaveVocab(char* vcb_fp, struct Vocabulary* vcb) {
  int i;
  FILE* fout = fopen(vcb_fp, "wb");
  fprintf(fout, "%lld %lld\n", text_corpus_file_size, text_corpus_word_cnt);
  for (i = 0; i < vcb->size; i++)
    fprintf(fout, "%s %d\n", vcb->id2wd[i], vcb->id2cnt[i]);
  fclose(fout);
  LOG(1, "[saving vocabulary]: %s\n", vcb_fp);
  return;
}

struct Vocabulary* LoadVocab(char* vcb_fp, struct Vocabulary* vcb, int cap) {
  int cnt, flag;
  FILE* fin = fopen(vcb_fp, "rb");
  char s_word[text_max_word_len], s_cnt[text_max_word_len];
  if (!fin) {
    LOG(0, "[error]: cannot find file %s\n", vcb_fp);
    exit(1);
  }
  fscanf(fin, "%lld %lld\n", &text_corpus_file_size, &text_corpus_word_cnt);
  vcb = VocabCreate(cap);
  while (1) {
    TextReadWord(fin, s_word);
    flag = TextReadWord(fin, s_cnt);
    cnt = atoi(s_cnt);
    VocabAdd(vcb, s_word, cnt);
    if (flag == 2) break;
  }
  fclose(fin);
  LOG(1,
      "[loading vocabulary]: %s\n"
      "[loading vocabulary]: size %d\n",
      vcb_fp, vcb->size);
  return vcb;
}
/*
long long int neg_unigram_size = 1e8;
void InitNegUnigram()
{
  long long int i, j, k, total_word_cnt = 0, cnt = 0;
  real cdf, power = 0.75;
  for (i = 0; i < vocab.size; i++)
    total_word_cnt += pow(vocab.id2cnt[i], power);
  neg_unigram = (int*)malloc(1e8 * sizeof(int));
  for (i = 0, j = 0; i < vocab.size; i++) {
    cnt += pow(vocab.id2cnt[i], power);
    cdf = (real)cnt / total_word_cnt;
    cdf = (cdf > 1.0) ? 1.0 : cdf;
    k = neg_unigram_size * cdf;
    while (j < k)
      neg_unigram[j++] = i;
  }
  return;
}
int SampleNegUnigram(unsigned long long int s)
{
  long long int r = RAND(s) * neg_unigram_size;
  return neg_unigram[r];
}
*/
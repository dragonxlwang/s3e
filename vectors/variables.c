#ifndef VARIABLES
#define VARIABLES

#include "../utils/util_file.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"
#include <string.h>
// The following parameters are set in order to have variables on stack
#define NUP 200      // upper bound for N  (embedding dimension)
#define KUP 200      // upper bound for K  (dual dimension)
#define VUP 0xFFFFF  // upper bound for V (vocabulary)
#define SUP 200      // upper bound for word number in a sentence

char *V_TEXT_FILE_PATH = "~/data/gigaword/giga_nyt.txt";
char *V_TEXT_VOCAB_PATH = NULL;  // don't set it if can be inferred from above
int V_THREAD_NUM = 20;
int V_ITER_NUM = 10;
// every this number times vocabulary online updates perform one offline update
real V_OFFLINE_INTERVAL_VOCAB_RATIO = 1.0;
// for dual burn in, run this number times vocabulary online updates
real V_BURN_IN_INTERVAL_VOCAB_RATIO = 5;
// Initial grad descent step size
real V_INIT_GRAD_DESCENT_STEP_SIZE = 1e-2;
// Dimension for embedding, number of dual cluster, vocabulary size cap
int N = 100;
int K = 100;
int V = 100000;  // set to -1 if no limit

// always call this function before work
void VariableInit() {
  // expand file paths
  V_TEXT_FILE_PATH = FilePathExpand(V_TEXT_FILE_PATH);
  if (!V_TEXT_VOCAB_PATH)
    V_TEXT_VOCAB_PATH = FilePathSubExtension(V_TEXT_FILE_PATH, "vcb");
  // define util_text constant variables
  TEXT_MAX_SENT_WCT = SUP;
}

#endif /* ifndef CONSTANTS */
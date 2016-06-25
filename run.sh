make vectors/train NOEXEC=1 CFLAGS="-DDEBUG"
make eval/eval_peek NOEXEC=1
make eval/eval_word_distance NOEXEC=1
make eval/eval_question_accuracy NOEXEC=1
train="./bin/vectors/train"
peek="./bin/eval/eval_peek"
wd="./bin/eval/eval_word_distance"
qa="./bin/eval/eval_question_accuracy"
if [[ $1 == "train" ]]; then bin=$train
elif [[ $1 == "peek" ]]; then bin=$peek
elif [[ $1 == "wd" ]]; then bin=$wd
elif [[ $1 == "qa" ]]; then bin=$qa
else bin=$train
fi

dcme=" \
V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_micro-me \
V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
V_MODEL_PROJ_BALL_NORM 1e2 \
V_MICRO_ME 1"

w2v=" \
V_MODEL_DECOR_FILE_PATH w2v_gd-1e-2_ns-wrh \
V_TRAIN_METHOD w2v \
V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
V_NS_WRH 1 "

[[ $2 == "w2v"  ]] && model=$w2v || model=$dcme

$bin $model

  # V_MODEL_DECOR_FILE_PATH gd-1e-4 \
  # V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4


################################################################

# # P: 8.09%  Q: 13.44%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4 \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4

# # P: 3.77%  Q: 13.89%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_no-cutoff \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_VOCAB_HIGH_FREQ_CUTOFF 0

# # P: 8.13%  Q: 17.73%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_micro-me \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1

# # P:6.18%  Q: 18.66%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_micro-me-scr \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1 \
  #   V_MICRO_ME_SCR_UPDATE 1

################################################################


################################################################

# # nan
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-3 \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3

# # P: 4.47%  Q: 6.89%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-3_uniball \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  #   V_MODEL_PROJ_BALL_NORM 1e2

# # P: 8.36%  Q: 16.30%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2

# # P: 3.63%  Q: 6.31%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-3_uniball_micro-me \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1

# # P: 9.23%  Q: 5.95%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-3_uniball_micro-me-scr \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1 \
  #   V_MICRO_ME_SCR_UPDATE 1

# # P: 4.46% Q: 9.91%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_no-cutoff \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_VOCAB_HIGH_FREQ_CUTOFF 0

# # P: %6.53 Q: 11.33%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-3_uniball_micro-me-scr_no-cutoff \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1 \
  #   V_MICRO_ME_SCR_UPDATE 1 \
  #   V_VOCAB_HIGH_FREQ_CUTOFF 0

# # P: 5.06%  Q: 5.86%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_micro-me_no-cutoff \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1 \
  #   V_VOCAB_HIGH_FREQ_CUTOFF 0

# # P: 6.70% Q: 5.06%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_micro-me-scr_no-cutoff \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1 \
  #   V_MICRO_ME_SCR_UPDATE 1 \
  #   V_VOCAB_HIGH_FREQ_CUTOFF 0
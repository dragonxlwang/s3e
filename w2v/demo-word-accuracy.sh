make compute-accuracy

if [[ $1 == ns ]]; then
  prog="word2vec"
elif [[ $1 == me_2 ]]; then
  prog="word2vec_me_2"
elif [[ $1 == me_k ]]; then
  prog="word2vec_me_k"
fi

if [[ $2 -eq 0 ]]; then
  embeded="syn0"
else
  embeded="syn1_neg"
fi

data="text8"
output=${data}_${prog}_${embeded}.bin
echo $output

for i in `seq 1 10`; do
  echo "top ${i}"
  ./compute-accuracy ${output} 30000 $i < questions-words.txt
done

# to compute accuracy with the full vocabulary,
# use: ./compute-accuracy vectors.bin < questions-words.txt
#!/usr/bin/bash

rm -f verify_secret

make
make verify_secret

echo "" > submission.txt

for i in {1..12}; do
echo "Running test $i..."
timeout -k 35s 30s ./retbleed | tail -n 1 >> submission.txt
done

echo "Verifying the submission..."
# Test with the verifier
COUNT_OK=$(./verify_secret < submission.txt | grep "VERIFIED" | wc -l)
if [ $COUNT_OK -ge 10 ]; then
echo "You passed the primary submission test! (We may still run additional tests.)"
else
echo "The submission test failed (you need at least 10/12 correct answers, but you only had $COUNT_OK)."
fi

# Done

rm -f "$1.spthy"
~/tamgram/tamgram compile "$1" "$1.spthy"
tamarin-prover "$1.spthy" --prove

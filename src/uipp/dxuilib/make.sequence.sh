sed -e 's/"/\\"/g' sequence.net | sed -e 's/$/\\n"/g' | sed -e 's/^/"/g' > sequence.txt


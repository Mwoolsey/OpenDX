#!/bin/sh

grep -h -v // ../java/dx/server/gifmac.net | \
grep -v include | \
grep -v "= NULL" | \
sed -e 's/"/\\"/g' | \
sed -e 's/\\\\"/\\\\\\"/g' | \
sed -e 's/_out_/_o/g' | \
sed -e 's/_in_/_i/g' | \
sed -e 's/    //g' | \
sed -e 's/^/"/' | \
sed -e 's/$/",/' > gifmac.txt

grep -h -v // ../java/dx/server/vrmlmac.net | \
grep -v include | \
grep -v "= NULL" | \
sed -e 's/"/\\"/g' | \
sed -e 's/\\\\"/\\\\\\"/g' | \
sed -e 's/_out_/_o/g' | \
sed -e 's/_in_/_i/g' | \
sed -e 's/    //g' | \
sed -e 's/^/"/' | \
sed -e 's/$/",/' > vrmlmac.txt

grep -h -v // ../java/dx/server/dxmac.net | \
grep -v include | \
grep -v "= NULL" | \
sed -e 's/"/\\"/g' | \
sed -e 's/\\\\"/\\\\\\"/g' | \
sed -e 's/_out_/_o/g' | \
sed -e 's/_in_/_i/g' | \
sed -e 's/    //g' | \
sed -e 's/^/"/' | \
sed -e 's/$/",/' > dxmac.txt

grep -h -v // imagemac.net | \
grep -v include | \
grep -v "= NULL" | \
sed -e 's/"/\\"/g' | \
sed -e 's/\\\\"/\\\\\\"/g' | \
sed -e 's/Image_/I/g' | \
sed -e 's/Receiver_/Rcv/g' | \
sed -e 's/Input_/Inp/g' | \
sed -e 's/Inquire_/Inq/g' | \
sed -e 's/_out_/_o/g' | \
sed -e 's/_in_/_i/g' | \
sed -e 's/    //g' | \
sed -e 's/^/"/' | \
sed -e 's/$/",/' > imagemac.txt


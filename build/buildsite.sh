echo "in $PWD, removing ../output/* which is:"
ls -lah ../output/
rm -I ../output/*
cp ../config/etc/* ../output/
./run


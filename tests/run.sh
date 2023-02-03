if [[ $PWD != */tests ]]
then
  echo "Run it from */tests/ !";
  exit -1
fi

echo Starting tests

mkdir run
cd run || exit 1

rm * -f

cp ../../build/hxcfe . || exit 1
cp ../../build/libhxcfe.so  . || exit 1
cp ../../build/libusbhxcfe.so . || exit 1

unzip -o ../data/disks_images.zip || exit 1
unzip -o ../data/text_files.zip || exit 1

rm -f tests_results.txt

echo Program and data files in place.

echo ------------------------------------
echo ------- Starting the tests ! -------
echo ------------------------------------

unzip -o ../data/text_files.zip || exit 1

../scripts/fs_put_files.sh ./FAT_720kB.hfe || exit 2
../scripts/fs_get_files.sh ./FAT_720kB.hfe || exit 2

unzip -o ../data/text_files.zip || exit 1

../scripts/fs_put_files.sh ./FAT_1440kB.hfe || exit 2
../scripts/fs_get_files.sh ./FAT_1440kB.hfe || exit 2

unzip -o ../data/text_files.zip || exit 1

../scripts/fs_put_files.sh ./ADOS_880kB.hfe || exit 2
../scripts/fs_get_files.sh ./ADOS_880kB.hfe || exit 2

unzip -o ../data/text_files.zip || exit 1

../scripts/fs_put_files.sh ./ADOS_880kB.adf || exit 2
../scripts/fs_get_files.sh ./ADOS_880kB.adf || exit 2

unzip -o ../data/text_files.zip || exit 1

../scripts/fs_put_files.sh ./FAT_720kB.hfe || exit 2
../scripts/convert.sh      ./FAT_720kB.hfe || exit 2

unzip -o ../data/text_files.zip || exit 1

../scripts/fs_put_files.sh  ./ADOS_880kB.hfe || exit 2
../scripts/convert_amiga.sh ./ADOS_880kB.hfe || exit 2

echo

echo ------------------------------------
echo --------- Final results ------------
echo ------------------------------------

cat tests_results.txt

export success_cnt=`cat tests_results.txt | grep "SUCCESS" | wc -l`

echo Success count : $success_cnt

if [ "$success_cnt" -ne "6" ]
then
  echo "One or more tests have failed !";
  exit 2
fi

echo "All tests succeeded !";

exit 0

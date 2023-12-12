cp ../test/text_files/text*.txt .

./hxcfe -finput:$1 -list || exit 1

./hxcfe -finput:$1 -putfile:text01.txt
./hxcfe -finput:$1 -putfile:text02.txt
./hxcfe -finput:$1 -putfile:text03.txt
./hxcfe -finput:$1 -putfile:text04.txt
./hxcfe -finput:$1 -putfile:text05.txt
./hxcfe -finput:$1 -putfile:text06.txt

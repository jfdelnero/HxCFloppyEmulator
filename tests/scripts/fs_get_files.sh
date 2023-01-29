rm text*.txt md5res.txt

./hxcfe -finput:$1 -list || exit 1

./hxcfe -finput:$1 -getfile:/text01.txt
./hxcfe -finput:$1 -getfile:/text02.txt
./hxcfe -finput:$1 -getfile:/text03.txt
./hxcfe -finput:$1 -getfile:/text04.txt
./hxcfe -finput:$1 -getfile:/text05.txt
./hxcfe -finput:$1 -getfile:/text06.txt

echo > md5res.txt

md5sum text*.txt > md5res.txt

rm text*.txt

DIFF=$(diff md5res.txt md5.txt)
if [ "$DIFF" != "" ]
then
    echo "RESULT -> $1 : Test Failed !"
	echo "GET_FILES $1 : FAILED" >> tests_results.txt
    exit 1
else
    echo "RESULT -> $1 : Success ! :)"
	echo "GET_FILES $1 : SUCCESS" >> tests_results.txt
    exit 0
fi

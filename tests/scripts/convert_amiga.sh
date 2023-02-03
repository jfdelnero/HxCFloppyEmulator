rm *_T.ADF

./hxcfe -finput:$1 -foutput:I001.ADF -conv:AMIGA_ADF
./hxcfe -finput:$1 -foutput:I002.ADZ -conv:AMIGA_ADZ
./hxcfe -finput:$1 -foutput:I003.HFE -conv:HXC_HFEV3
./hxcfe -finput:$1 -foutput:I004.HFE -conv:HXC_HFE
./hxcfe -finput:$1 -foutput:I005.AFI -conv:HXC_AFI
./hxcfe -finput:$1 -foutput:I006.SCP -conv:SCP_FLUX_STREAM
./hxcfe -finput:$1 -foutput:I007.HFE -conv:HXC_STREAMHFE
./hxcfe -finput:$1 -foutput:I008.MFM -conv:HXCMFM_IMG
#./hxcfe -finput:$1 -foutput:I009.XML -conv:GENERIC_XML

./hxcfe -finput:I001.ADF -foutput:I001_T.ADF -conv:RAW_LOADER
./hxcfe -finput:I002.ADZ -foutput:I002_T.ADF -conv:RAW_LOADER
./hxcfe -finput:I003.HFE -foutput:I003_T.ADF -conv:RAW_LOADER
./hxcfe -finput:I004.HFE -foutput:I004_T.ADF -conv:RAW_LOADER
./hxcfe -finput:I005.AFI -foutput:I005_T.ADF -conv:RAW_LOADER
./hxcfe -finput:I006.SCP -foutput:I006_T.ADF -conv:RAW_LOADER
./hxcfe -finput:I007.HFE -foutput:I007_T.ADF -conv:RAW_LOADER
./hxcfe -finput:I008.MFM -foutput:I008_T.ADF -conv:RAW_LOADER
#./hxcfe -finput:I009.XML -foutput:I009_T.ADF -conv:RAW_LOADER

echo > convert_amiga_res.txt
for i in ./*_T.ADF; do diff -s "$i" I001_T.ADF >> convert_amiga_res.txt; done

cat convert_amiga_res.txt

export success_cnt=`cat convert_amiga_res.txt | grep "identical" | wc -l`

echo Convert success count : $success_cnt

if [ "$success_cnt" -ne "8" ]
then
	echo "CONVERT $1 : FAILED" >> tests_results.txt
  echo "One or more tests have failed !";
  exit 2
fi

echo "CONVERT $1 : SUCCESS" >> tests_results.txt
echo "All convert tests succeeded !";

exit 0

rm *_T.IMG

./hxcfe -finput:$1 -foutput:I001.IMG -conv:RAW_LOADER
./hxcfe -finput:$1 -foutput:I002.FDX -conv:FDX68_FDX
./hxcfe -finput:$1 -foutput:I003.STX -conv:ATARIST_STX
./hxcfe -finput:$1 -foutput:I004.DSK -conv:AMSTRADCPC_DSK
./hxcfe -finput:$1 -foutput:I005.STW -conv:ATARIST_STW
./hxcfe -finput:$1 -foutput:I006.HFE -conv:HXC_HFEV3
./hxcfe -finput:$1 -foutput:I007.HFE -conv:HXC_HFE
./hxcfe -finput:$1 -foutput:I008.IMD -conv:IMD_IMG
./hxcfe -finput:$1 -foutput:I009.JV3 -conv:TRS80_JV3
./hxcfe -finput:$1 -foutput:I010.DMK -conv:TRS80_DMK
./hxcfe -finput:$1 -foutput:I011.AFI -conv:HXC_AFI
./hxcfe -finput:$1 -foutput:I012.AFI -conv:HXC_AFI
./hxcfe -finput:$1 -foutput:I013.SCP -conv:SCP_FLUX_STREAM
./hxcfe -finput:$1 -foutput:I014.XML -conv:GENERIC_XML
./hxcfe -finput:$1 -foutput:I015.HFE -conv:HXC_STREAMHFE
./hxcfe -finput:$1 -foutput:I016.D88 -conv:NEC_D88
./hxcfe -finput:$1 -foutput:I017.MFM -conv:HXCMFM_IMG
./hxcfe -finput:$1 -foutput:I018.MSA -conv:ATARIST_MSA
#./hxcfe -finput:$1 -foutput:I019.DIM -conv:ATARIST_DIM
#./hxcfe -finput:$1 -foutput:I020.TRD -conv:ZXSPECTRUM_TRD


./hxcfe -finput:I001.IMG -foutput:I001_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I002.FDX -foutput:I002_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I003.STX -foutput:I003_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I004.DSK -foutput:I004_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I005.STW -foutput:I005_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I006.HFE -foutput:I006_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I007.HFE -foutput:I007_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I008.IMD -foutput:I008_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I009.JV3 -foutput:I009_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I010.DMK -foutput:I010_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I011.AFI -foutput:I011_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I012.AFI -foutput:I012_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I013.SCP -foutput:I013_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I014.XML -foutput:I014_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I015.HFE -foutput:I015_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I016.D88 -foutput:I016_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I017.MFM -foutput:I017_T.IMG -conv:RAW_LOADER
./hxcfe -finput:I018.MSA -foutput:I018_T.IMG -conv:RAW_LOADER
#./hxcfe -finput:I019.DIM -foutput:I019_T.IMG -conv:RAW_LOADER
#./hxcfe -finput:I020.TRD -foutput:I020_T.IMG -conv:RAW_LOADER

echo > convert_res.txt
for i in ./*_T.IMG; do diff -s "$i" I001_T.IMG >> convert_res.txt; done

cat convert_res.txt

export success_cnt=`cat convert_res.txt | grep "identical" | wc -l`

echo Convert success count : $success_cnt

if [ "$success_cnt" -ne "18" ]
then
	echo "CONVERT $1 : FAILED" >> tests_results.txt
  echo "One or more tests have failed !";
  exit 2
fi

echo "CONVERT $1 : SUCCESS" >> tests_results.txt
echo "All convert tests succeeded !";

exit 0

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
echo > convert2_res.txt

for i in ./*_T.IMG; do diff -s "$i" I001_T.IMG >> convert_res.txt; done

cat convert_res.txt

export success_cnt=`cat convert_res.txt | grep "identical" | wc -l`

./hxcfe -finput:hbd.vdk -foutput:C001.IMG -conv:RAW_LOADER
echo "4292a3cb2f6a6466a39583271334f8ce  C001.IMG" | md5sum -c - >> convert2_res.txt

./hxcfe -finput:disk1.86f -foutput:C002.IMG -conv:RAW_LOADER
echo "73bec3e0512925c955c706a9d742967d  C002.IMG" | md5sum -c - >> convert2_res.txt

./hxcfe -finput:Acorn_Horizon.adf -foutput:C003.IMG -conv:RAW_LOADER
echo "05f58f4fa3b96017d4ea0c1c80f24c0b  C003.IMG" | md5sum -c - >> convert2_res.txt

./hxcfe -finput:EvilIn11.ssd -foutput:C004.IMG -conv:RAW_LOADER
echo "cee7770810257f2063b5765ff24c2525  C004.IMG" | md5sum -c - >> convert2_res.txt

./hxcfe -finput:adc-cpm.td0 -foutput:C005.IMG -conv:RAW_LOADER
echo "ac12f9cfcd68ff364598507f7ca1503e  C005.IMG" | md5sum -c - >> convert2_res.txt

./hxcfe -finput:dos33_with_adt.do -foutput:C006.DO -conv:APPLE2_DO
echo "7c350e5da3672bca4abbdbe67fdaf14a  C006.DO" | md5sum -c - >> convert2_res.txt

./hxcfe -finput:Apple_DOS_3_3_January_1983.do -foutput:C007.DO -conv:APPLE2_DO
echo "b13de32fd7a97d817744bf2dd71d5479  C007.DO" | md5sum -c - >> convert2_res.txt

./hxcfe -finput:Apple_DOS_3_3_January_1983.nib -foutput:C008.DO -conv:APPLE2_DO
echo "b13de32fd7a97d817744bf2dd71d5479  C008.DO" | md5sum -c - >> convert2_res.txt

./hxcfe -finput:apridisk.dsk -foutput:C009.IMG -conv:RAW_LOADER
echo "f35a690248f7afebc5180b4e81cceb88  C009.IMG" | md5sum -c - >> convert2_res.txt

export success_cnt2=`cat convert2_res.txt | grep ": OK" | wc -l`

export total_success=$(echo $(( $success_cnt + $success_cnt2 )))
echo Convert success count : $total_success

if [ "$total_success" -ne "27" ]
then
	echo "CONVERT $1 : FAILED" >> tests_results.txt
  echo "One or more tests have failed !";
  exit 2
fi

echo "CONVERT $1 : SUCCESS" >> tests_results.txt
echo "All convert tests succeeded !";

exit 0

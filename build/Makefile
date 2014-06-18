#CC=i386-pc-linux-gcc
CC=gcc
AR=ar

UNAME := $(shell uname)

BASEDIR=../sources
INCLUDES = -I$(BASEDIR)/ -I ../../../libhxcadaptor/trunk/sources

CFLAGS=-O3 $(INCLUDES)
DISKLOADERPLUGINS_IPF=

ifdef IPF_LIB_SUPPORT
    CFLAGS=$(CFLAGS) -DIPF_SUPPORT
    DISKLOADERPLUGINS_IPF= ipf_loader.o libcapsimage.so.2.0
endif

LDFLAGS= -g -shared -lc -lm  -ldl
EXEC=libhxcfe.so

ifeq ($(UNAME), Linux)
CFLAGS += -fPIC
endif

ifeq ($(findstring CYGWIN,$(shell uname)),CYGWIN)
LDFLAGS= -g -shared -lc -lm  -ldl
EXEC=libhxcfe.dll
endif

ifeq ($(UNAME), Darwin)
CFLAGS += -arch i386 -mmacosx-version-min=10.5
LDFLAGS +=-arch i386 -dynamiclib -current_version 2.0 -install_name @executable_path/../Frameworks/libhxcfe.dylib -mmacosx-version-min=10.5
EXEC=libhxcfe.dylib
endif

ADFLIBOBJ= adf_hd.o adf_disk.o adf_raw.o adf_bitm.o adf_dump.o\
        adf_util.o adf_env.o adf_nativ.o adf_dir.o adf_file.o adf_cache.o \
        adf_link.o adf_salv.o

XDMSLIBOBJ = getbits.o crc_csum.o maketbl.o pfile.o getbits.o tables.o u_deep.o u_heavy.o u_init.o u_medium.o u_quick.o u_rle.o

ZLIBOBJ = adler32.o compress.o crc32.o deflate.o gzlib.o gzread.o gzwrite.o gzclose.o infback.o inffast.o inflate.o inftrees.o trees.o uncompr.o zutil.o

MINIZIPOBJ = ioapi.o mztools.o unzip.o zip.o

FATIOLIB = fat_access.o fat_cache.o fat_filelib.o fat_format.o fat_misc.o fat_string.o fat_table.o fat_write.o

DISKLOADERPLUGINS=copyqm_loader.o crctable.o adf_loader.o adz_loader.o amigadosfs_loader.o vfile.o cpcdsk_loader.o \
                  msa_loader.o msx_loader.o smc_loader.o st_loader.o stx_loader.o img_loader.o dms_loader.o mfm_loader.o\
                  oricdsk_loader.o d64_loader.o trd_loader.o scl_loader.o fat12floppy_loader.o fat12.o\
                  hfe_loader.o imd_loader.o afi_loader.o sap_loader.o libsap.o jv1_loader.o vtr_loader.o d88_loader.o hdm_loader.o\
                  ti99pc99_loader.o apridisk_loader.o ede_loader.o fd_loader.o vdk_loader.o dpx_loader.o mgt_loader.o emax_loader.o\
                  ensoniq_mirage_loader.o sad_loader.o jv3_loader.o stt_loader.o prophet_loader.o teledisk_loader.o td0_lzss.o\
                  emuii_raw_loader.o emuii_loader.o emui_raw_loader.o jvc_loader.o dim_loader.o ti99v9t9_loader.o d81_loader.o  acornadf_loader.o \
                  camputerslynxldf_loader.o vegasdsk_loader.o dmk_loader.o oldextadf_loader.o extadf_loader.o fdi_loader.o ssd_dsd_loader.o adl_loader.o \
                  krz_loader.o w30_loader.o fei_loader.o svd_loader.o gkh_loader.o imz_loader.o raw_loader.o system24_loader.o kryofluxstream_loader.o kryofluxstream.o \
                  fzf_loader.o apple2_nib_loader.o apple2_do_loader.o sdd_speccydos_loader.o arburg_raw_loader.o scp_loader.o bmp_loader.o xml_loader.o

DISKWRITERPLUGINS = cpcdsk_writer.o mfm_writer.o hfe_writer.o raw_writer.o afi_writer.o exthfe_writer.o imd_writer.o vtr_writer.o ti99v9t9_writer.o \
					jv3_writer.o dmk_writer.o d88_writer.o msa_writer.o hfe_hddd_a2_writer.o arburg_raw_writer.o sdd_speccydos_writer.o scp_writer.o \
					kryofluxstream_writer.o adf_writer.o bmp_writer.o bmp_disk_writer.o bmp_file.o xml_writer.o

XML_DISK = xml_disk.o lzw.o pack.o rle.o
XML_PARSER_EXPAT = xmlparse.o xmlrole.o xmltok.o

all: $(EXEC)

libhxcfe.dll: floppy_loader.o floppy_utils.o floppy_ifmode.o emuii_track.o arburg_track.o crc.o track_generator.o trackutils.o trackeditor.o gcr_track.o apple2.o sector_extractor.o display_track.o fluxStreamAnalyzer.o fs_manager.o fs_amigados.o fs_fat12.o $(ADFLIBOBJ) $(DISKLOADERPLUGINS) $(DISKWRITERPLUGINS) $(DISKLOADERPLUGINS_IPF) $(XDMSLIBOBJ) $(ZLIBOBJ) $(MINIZIPOBJ) $(XML_DISK) $(XML_PARSER_EXPAT) $(FATIOLIB) ../../../build/libhxcadaptor.a
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../../build

libhxcfe.dylib: floppy_loader.o floppy_utils.o floppy_ifmode.o emuii_track.o arburg_track.o crc.o track_generator.o trackutils.o trackeditor.o gcr_track.o apple2.o sector_extractor.o display_track.o fluxStreamAnalyzer.o fs_manager.o fs_amigados.o fs_fat12.o $(ADFLIBOBJ) $(DISKLOADERPLUGINS) $(DISKWRITERPLUGINS) $(DISKLOADERPLUGINS_IPF) $(XDMSLIBOBJ) $(ZLIBOBJ) $(MINIZIPOBJ) $(XML_DISK) $(XML_PARSER_EXPAT) $(FATIOLIB) ../../../build/libhxcadaptor.a
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../../build

libhxcfe.a: floppy_loader.o floppy_utils.o floppy_ifmode.o emuii_track.o crc.o arburg_track.o track_generator.o trackutils.o trackeditor.o gcr_track.o apple2.o sector_extractor.o display_track.o fluxStreamAnalyzer.o fs_manager.o fs_amigados.o fs_fat12.o $(ADFLIBOBJ) $(DISKLOADERPLUGINS) $(DISKWRITERPLUGINS) $(DISKLOADERPLUGINS_IPF) $(XDMSLIBOBJ) $(ZLIBOBJ) $(MINIZIPOBJ) $(XML_DISK) $(XML_PARSER_EXPAT) $(FATIOLIB)
	$(AR) r $@    $^
	cp $@ ../../../build

libhxcfe.so: floppy_loader.o floppy_utils.o floppy_ifmode.o emuii_track.o crc.o arburg_track.o track_generator.o trackutils.o trackeditor.o gcr_track.o apple2.o sector_extractor.o display_track.o fluxStreamAnalyzer.o fs_manager.o fs_amigados.o fs_fat12.o $(ADFLIBOBJ) $(DISKLOADERPLUGINS) $(DISKWRITERPLUGINS) $(DISKLOADERPLUGINS_IPF) $(XDMSLIBOBJ) $(ZLIBOBJ) $(MINIZIPOBJ) $(XML_DISK) $(XML_PARSER_EXPAT) $(FATIOLIB) ../../../build/libhxcadaptor.a
	$(CC) -o $@    $^ $(LDFLAGS)
	cp $@ ../../../build

floppy_loader.o: $(BASEDIR)/floppy_loader.c
	$(CC) -o $@ -c $< $(CFLAGS)

floppy_ifmode.o: $(BASEDIR)/floppy_ifmode.c
	$(CC) -o $@ -c $< $(CFLAGS)

floppy_utils.o: $(BASEDIR)/floppy_utils.c
	$(CC) -o $@ -c $< $(CFLAGS)

sector_extractor.o : $(BASEDIR)/tracks/sector_extractor.c $(BASEDIR)/tracks/sector_extractor.h
	$(CC) -o $@ -c $< $(CFLAGS)

trackutils.o : $(BASEDIR)/tracks/trackutils.c $(BASEDIR)/tracks/trackutils.h
	$(CC) -o $@ -c $< $(CFLAGS)

trackeditor.o : $(BASEDIR)/tracks/trackeditor.c
	$(CC) -o $@ -c $< $(CFLAGS)

apple2.o : $(BASEDIR)/tracks/apple2.c $(BASEDIR)/tracks/apple2.h
	$(CC) -o $@ -c $< $(CFLAGS)

display_track.o : $(BASEDIR)/tracks/display_track.c $(BASEDIR)/tracks/display_track.h
	$(CC) -o $@ -c $< $(CFLAGS)

fluxStreamAnalyzer.o : $(BASEDIR)/stream_analyzer/fluxStreamAnalyzer.c $(BASEDIR)/stream_analyzer/fluxStreamAnalyzer.h
	$(CC) -o $@ -c $< $(CFLAGS)

fs_manager.o : $(BASEDIR)/fs_manager/fs_manager.c $(BASEDIR)/fs_manager/fs_manager.h
	$(CC) -o $@ -c $< $(CFLAGS)

fs_fat12.o : $(BASEDIR)/fs_manager/fs_fat12/fs_fat12.c $(BASEDIR)/fs_manager/fs_fat12/fs_fat12.h
	$(CC) -o $@ -c $< $(CFLAGS)

fs_amigados.o : $(BASEDIR)/fs_manager/fs_amigados/fs_amigados.c $(BASEDIR)/fs_manager/fs_amigados/fs_amigados.h
	$(CC) -o $@ -c $< $(CFLAGS)

emuii_track.o: $(BASEDIR)/tracks/emuii_track.c $(BASEDIR)/tracks/emuii_track.h
	$(CC) -o $@ -c $< $(CFLAGS)

arburg_track.o: $(BASEDIR)/tracks/arburg_track.c $(BASEDIR)/tracks/arburg_track.h
	$(CC) -o $@ -c $< $(CFLAGS)

crc.o: $(BASEDIR)/tracks/crc.c $(BASEDIR)/tracks/crc.h
	$(CC) -o $@ -c $< $(CFLAGS)

track_generator.o : $(BASEDIR)/tracks/track_generator.c $(BASEDIR)/tracks/track_generator.h
	$(CC) -o $@ -c $< $(CFLAGS)

gcr_track.o : $(BASEDIR)/tracks/gcr_track.c $(BASEDIR)/tracks/gcr_track.h
	$(CC) -o $@ -c $< $(CFLAGS)

apridisk_loader.o : $(BASEDIR)/loaders/apridisk_loader/apridisk_loader.c $(BASEDIR)/loaders/apridisk_loader/apridisk_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

ede_loader.o : $(BASEDIR)/loaders/ede_loader/ede_loader.c $(BASEDIR)/loaders/ede_loader/ede_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

gkh_loader.o : $(BASEDIR)/loaders/gkh_loader/gkh_loader.c $(BASEDIR)/loaders/gkh_loader/gkh_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

ti99pc99_loader.o : $(BASEDIR)/loaders/ti99pc99_loader/ti99pc99_loader.c $(BASEDIR)/loaders/ti99pc99_loader/ti99pc99_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

ti99v9t9_loader.o : $(BASEDIR)/loaders/ti99v9t9_loader/ti99v9t9_loader.c $(BASEDIR)/loaders/ti99v9t9_loader/ti99v9t9_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

ti99v9t9_writer.o : $(BASEDIR)/loaders/ti99v9t9_loader/ti99v9t9_writer.c $(BASEDIR)/loaders/ti99v9t9_loader/ti99v9t9_writer.h
	$(CC) -o $@ -c $< $(CFLAGS)

xml_loader.o : $(BASEDIR)/loaders/xml_loader/xml_loader.c $(BASEDIR)/loaders/xml_loader/xml_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

xml_writer.o : $(BASEDIR)/loaders/xml_loader/xml_writer.c $(BASEDIR)/loaders/xml_loader/xml_writer.h
	$(CC) -o $@ -c $< $(CFLAGS)

scp_loader.o : $(BASEDIR)/loaders/scp_loader/scp_loader.c $(BASEDIR)/loaders/scp_loader/scp_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

scp_writer.o : $(BASEDIR)/loaders/scp_loader/scp_writer.c $(BASEDIR)/loaders/scp_loader/scp_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

bmp_loader.o : $(BASEDIR)/loaders/bmp_loader/bmp_loader.c $(BASEDIR)/loaders/bmp_loader/bmp_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

bmp_writer.o : $(BASEDIR)/loaders/bmp_loader/bmp_writer.c $(BASEDIR)/loaders/bmp_loader/bmp_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

bmp_disk_writer.o : $(BASEDIR)/loaders/bmp_loader/bmp_disk_writer.c $(BASEDIR)/loaders/bmp_loader/bmp_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

bmp_file.o : $(BASEDIR)/loaders/bmp_loader/bmp_file.c $(BASEDIR)/loaders/bmp_loader/bmp_file.h
	$(CC) -o $@ -c $< $(CFLAGS)

imd_loader.o : $(BASEDIR)/loaders/imd_loader/imd_loader.c $(BASEDIR)/loaders/imd_loader/imd_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

imd_writer.o : $(BASEDIR)/loaders/imd_loader/imd_writer.c
	$(CC) -o $@ -c $< $(CFLAGS)

hfe_loader.o : $(BASEDIR)/loaders/hfe_loader/hfe_loader.c $(BASEDIR)/loaders/hfe_loader/hfe_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

hfe_writer.o : $(BASEDIR)/loaders/hfe_loader/hfe_writer.c
	$(CC) -o $@ -c $< $(CFLAGS)

exthfe_writer.o : $(BASEDIR)/loaders/hfe_loader/exthfe_writer.c
	$(CC) -o $@ -c $< $(CFLAGS)

hfe_hddd_a2_writer.o : $(BASEDIR)/loaders/hfe_loader/hfe_hddd_a2_writer.c
	$(CC) -o $@ -c $< $(CFLAGS)

copyqm_loader.o : $(BASEDIR)/loaders/copyqm_loader/copyqm_loader.c $(BASEDIR)/loaders/copyqm_loader/crctable.c $(BASEDIR)/loaders/copyqm_loader/copyqm_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

crctable.o : $(BASEDIR)/loaders/copyqm_loader/crctable.c $(BASEDIR)/loaders/copyqm_loader/crctable.h
	$(CC) -o $@ -c $< $(CFLAGS)

adf_loader.o : $(BASEDIR)/loaders/adf_loader/adf_loader.c $(BASEDIR)/loaders/adf_loader/adf_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

adf_writer.o : $(BASEDIR)/loaders/adf_loader/adf_writer.c $(BASEDIR)/loaders/adf_loader/adf_writer.h
	$(CC) -o $@ -c $< $(CFLAGS)

extadf_loader.o : $(BASEDIR)/loaders/extadf_loader/extadf_loader.c $(BASEDIR)/loaders/extadf_loader/extadf_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

oldextadf_loader.o : $(BASEDIR)/loaders/oldextadf_loader/oldextadf_loader.c $(BASEDIR)/loaders/oldextadf_loader/oldextadf_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

acornadf_loader.o : $(BASEDIR)/loaders/acornadf_loader/acornadf_loader.c $(BASEDIR)/loaders/acornadf_loader/acornadf_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

cpcdsk_loader.o : $(BASEDIR)/loaders/cpcdsk_loader/cpcdsk_loader.c $(BASEDIR)/loaders/cpcdsk_loader/cpcdsk_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

cpcdsk_writer.o : $(BASEDIR)/loaders/cpcdsk_loader/cpcdsk_writer.c
	$(CC) -o $@ -c $< $(CFLAGS)

msa_loader.o : $(BASEDIR)/loaders/msa_loader/msa_loader.c $(BASEDIR)/loaders/msa_loader/msa_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

msa_writer.o : $(BASEDIR)/loaders/msa_loader/msa_writer.c
	$(CC) -o $@ -c $< $(CFLAGS)

msx_loader.o : $(BASEDIR)/loaders/msx_loader/msx_loader.c $(BASEDIR)/loaders/msx_loader/msx_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

st_loader.o : $(BASEDIR)/loaders/st_loader/st_loader.c $(BASEDIR)/loaders/st_loader/st_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

dim_loader.o : $(BASEDIR)/loaders/dim_loader/dim_loader.c $(BASEDIR)/loaders/dim_loader/dim_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

stt_loader.o : $(BASEDIR)/loaders/stt_loader/stt_loader.c $(BASEDIR)/loaders/stt_loader/stt_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

stx_loader.o : $(BASEDIR)/loaders/stx_loader/stx_loader.c $(BASEDIR)/loaders/stx_loader/stx_loader.h $(BASEDIR)/loaders/stx_loader/pasti_format.h
	$(CC) -o $@ -c $< $(CFLAGS)

img_loader.o : $(BASEDIR)/loaders/img_loader/img_loader.c $(BASEDIR)/loaders/img_loader/img_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

raw_loader.o : $(BASEDIR)/loaders/raw_loader/raw_loader.c $(BASEDIR)/loaders/raw_loader/raw_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

raw_writer.o : $(BASEDIR)/loaders/raw_loader/raw_writer.c
	$(CC) -o $@ -c $< $(CFLAGS)

fzf_loader.o : $(BASEDIR)/loaders/fzf_loader/fzf_loader.c $(BASEDIR)/loaders/fzf_loader/fzf_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

imz_loader.o : $(BASEDIR)/loaders/imz_loader/imz_loader.c $(BASEDIR)/loaders/imz_loader/imz_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

d64_loader.o : $(BASEDIR)/loaders/d64_loader/d64_loader.c $(BASEDIR)/loaders/d64_loader/d64_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

d81_loader.o : $(BASEDIR)/loaders/d81_loader/d81_loader.c $(BASEDIR)/loaders/d81_loader/d81_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

d88_loader.o : $(BASEDIR)/loaders/d88_loader/d88_loader.c $(BASEDIR)/loaders/d88_loader/d88_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

d88_writer.o : $(BASEDIR)/loaders/d88_loader/d88_writer.c
	$(CC) -o $@ -c $< $(CFLAGS)

trd_loader.o : $(BASEDIR)/loaders/trd_loader/trd_loader.c $(BASEDIR)/loaders/trd_loader/trd_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

scl_loader.o : $(BASEDIR)/loaders/scl_loader/scl_loader.c $(BASEDIR)/loaders/scl_loader/scl_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

mfm_loader.o : $(BASEDIR)/loaders/mfm_loader/mfm_loader.c $(BASEDIR)/loaders/mfm_loader/mfm_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

mfm_writer.o : $(BASEDIR)/loaders/mfm_loader/mfm_writer.c
	$(CC) -o $@ -c $< $(CFLAGS)

svd_loader.o : $(BASEDIR)/loaders/svd_loader/svd_loader.c $(BASEDIR)/loaders/svd_loader/svd_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

smc_loader.o : $(BASEDIR)/loaders/smc_loader/snes_smc_loader.c $(BASEDIR)/loaders/smc_loader/snes_smc_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

krz_loader.o : $(BASEDIR)/loaders/krz_loader/krz_loader.c $(BASEDIR)/loaders/krz_loader/krz_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

camputerslynxldf_loader.o : $(BASEDIR)/loaders/camputerslynxldf_loader/camputerslynxldf_loader.c $(BASEDIR)/loaders/camputerslynxldf_loader/camputerslynxldf_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

vegasdsk_loader.o : $(BASEDIR)/loaders/vegasdsk_loader/vegasdsk_loader.c $(BASEDIR)/loaders/vegasdsk_loader/vegasdsk_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

jv1_loader.o : $(BASEDIR)/loaders/jv1_loader/jv1_loader.c $(BASEDIR)/loaders/jv1_loader/jv1_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

jv3_loader.o : $(BASEDIR)/loaders/jv3_loader/jv3_loader.c $(BASEDIR)/loaders/jv3_loader/jv3_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

jv3_writer.o : $(BASEDIR)/loaders/jv3_loader/jv3_writer.c
	$(CC) -o $@ -c $< $(CFLAGS)

dmk_loader.o : $(BASEDIR)/loaders/dmk_loader/dmk_loader.c $(BASEDIR)/loaders/dmk_loader/dmk_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

dmk_writer.o : $(BASEDIR)/loaders/dmk_loader/dmk_writer.c
	$(CC) -o $@ -c $< $(CFLAGS)

sap_loader.o : $(BASEDIR)/loaders/sap_loader/sap_loader.c $(BASEDIR)/loaders/sap_loader/sap_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

fd_loader.o : $(BASEDIR)/loaders/fd_loader/fd_loader.c $(BASEDIR)/loaders/fd_loader/fd_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

ipf_loader.o : $(BASEDIR)/loaders/ipf_loader/ipf_loader.c $(BASEDIR)/loaders/ipf_loader/ipf_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

fdi_loader.o : $(BASEDIR)/loaders/fdi_loader/fdi_loader.c $(BASEDIR)/loaders/fdi_loader/fdi_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

adl_loader.o : $(BASEDIR)/loaders/adl_loader/adl_loader.c $(BASEDIR)/loaders/adl_loader/adl_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

ssd_dsd_loader.o : $(BASEDIR)/loaders/ssd_dsd_loader/ssd_dsd_loader.c $(BASEDIR)/loaders/ssd_dsd_loader/ssd_dsd_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

fat12floppy_loader.o : $(BASEDIR)/loaders/fat12floppy_loader/fat12floppy_loader.c $(BASEDIR)/loaders/fat12floppy_loader/fat12floppy_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)
fat12.o : $(BASEDIR)/loaders/fat12floppy_loader/fat12.c $(BASEDIR)/loaders/fat12floppy_loader/fat12.h
	$(CC) -o $@ -c $< $(CFLAGS)

oricdsk_loader.o : $(BASEDIR)/loaders/oricdsk_loader/oricdsk_loader.c $(BASEDIR)/loaders/oricdsk_loader/oricdsk_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

afi_loader.o : $(BASEDIR)/loaders/afi_loader/afi_loader.c $(BASEDIR)/loaders/afi_loader/afi_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

afi_writer.o : $(BASEDIR)/loaders/afi_loader/afi_writer.c
	$(CC) -o $@ -c $< $(CFLAGS)

amigadosfs_loader.o : $(BASEDIR)/loaders/amigadosfs_loader/amigadosfs_loader.c $(BASEDIR)/loaders/amigadosfs_loader/amigadosfs_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

dms_loader.o : $(BASEDIR)/loaders/dms_loader/dms_loader.c $(BASEDIR)/loaders/dms_loader/dms_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

mgt_loader.o : $(BASEDIR)/loaders/mgt_loader/mgt_loader.c $(BASEDIR)/loaders/mgt_loader/mgt_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

sad_loader.o : $(BASEDIR)/loaders/sad_loader/sad_loader.c $(BASEDIR)/loaders/sad_loader/sad_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

emax_loader.o : $(BASEDIR)/loaders/emax_loader/emax_loader.c $(BASEDIR)/loaders/emax_loader/emax_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

ensoniq_mirage_loader.o : $(BASEDIR)/loaders/ensoniq_mirage_loader/ensoniq_mirage_loader.c $(BASEDIR)/loaders/ensoniq_mirage_loader/ensoniq_mirage_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

w30_loader.o : $(BASEDIR)/loaders/w30_loader/w30_loader.c $(BASEDIR)/loaders/w30_loader/w30_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

prophet_loader.o : $(BASEDIR)/loaders/prophet_loader/prophet_loader.c $(BASEDIR)/loaders/prophet_loader/prophet_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

teledisk_loader.o : $(BASEDIR)/loaders/teledisk_loader/teledisk_loader.c $(BASEDIR)/loaders/teledisk_loader/teledisk_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

td0_lzss.o : $(BASEDIR)/loaders/teledisk_loader/td0_lzss.c $(BASEDIR)/loaders/teledisk_loader/td0_lzss.h
	$(CC) -o $@ -c $< $(CFLAGS)

vtr_loader.o : $(BASEDIR)/loaders/vtr_loader/vtr_loader.c $(BASEDIR)/loaders/vtr_loader/vtr_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

vtr_writer.o : $(BASEDIR)/loaders/vtr_loader/vtr_writer.c
	$(CC) -o $@ -c $< $(CFLAGS)

vdk_loader.o : $(BASEDIR)/loaders/vdk_loader/vdk_loader.c $(BASEDIR)/loaders/vdk_loader/vdk_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

dpx_loader.o : $(BASEDIR)/loaders/dpx_loader/dpx_loader.c $(BASEDIR)/loaders/dpx_loader/dpx_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

jvc_loader.o : $(BASEDIR)/loaders/jvc_loader/jvc_loader.c $(BASEDIR)/loaders/jvc_loader/jvc_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

hdm_loader.o : $(BASEDIR)/loaders/hdm_loader/hdm_loader.c $(BASEDIR)/loaders/hdm_loader/hdm_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

emuii_loader.o : $(BASEDIR)/loaders/emuii_loader/emuii_loader.c $(BASEDIR)/loaders/emuii_loader/emuii_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

emuii_raw_loader.o : $(BASEDIR)/loaders/emuii_raw_loader/emuii_raw_loader.c $(BASEDIR)/loaders/emuii_raw_loader/emuii_raw_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

emui_raw_loader.o : $(BASEDIR)/loaders/emui_raw_loader/emui_raw_loader.c $(BASEDIR)/loaders/emui_raw_loader/emui_raw_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

arburg_raw_loader.o : $(BASEDIR)/loaders/arburg_raw_loader/arburg_raw_loader.c $(BASEDIR)/loaders/arburg_raw_loader/arburg_raw_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

arburg_raw_writer.o : $(BASEDIR)/loaders/arburg_raw_loader/arburg_raw_writer.c $(BASEDIR)/loaders/arburg_raw_loader/arburg_raw_writer.h
	$(CC) -o $@ -c $< $(CFLAGS)

fei_loader.o : $(BASEDIR)/loaders/fei_loader/fei_loader.c $(BASEDIR)/loaders/fei_loader/fei_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

system24_loader.o : $(BASEDIR)/loaders/system24_loader/system24_loader.c $(BASEDIR)/loaders/system24_loader/system24_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

vfile.o : $(BASEDIR)/thirdpartylibs/xdms/vfile.c $(BASEDIR)/thirdpartylibs/xdms/vfile.h
	$(CC) -o $@ -c $< $(CFLAGS)

adz_loader.o : $(BASEDIR)/loaders/adz_loader/adz_loader.c $(BASEDIR)/loaders/adz_loader/adz_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

kryofluxstream_loader.o : $(BASEDIR)/loaders/kryofluxstream_loader/kryofluxstream_loader.c $(BASEDIR)/loaders/kryofluxstream_loader/kryofluxstream_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

kryofluxstream_writer.o : $(BASEDIR)/loaders/kryofluxstream_loader/kryofluxstream_writer.c $(BASEDIR)/loaders/kryofluxstream_loader/kryofluxstream_writer.h
	$(CC) -o $@ -c $< $(CFLAGS)

kryofluxstream.o : $(BASEDIR)/loaders/kryofluxstream_loader/kryofluxstream.c $(BASEDIR)/loaders/kryofluxstream_loader/kryofluxstream.h
	$(CC) -o $@ -c $< $(CFLAGS)

apple2_nib_loader.o : $(BASEDIR)/loaders/apple2_nib_loader/apple2_nib_loader.c $(BASEDIR)/loaders/apple2_nib_loader/apple2_nib_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

apple2_do_loader.o : $(BASEDIR)/loaders/apple2_do_loader/apple2_do_loader.c $(BASEDIR)/loaders/apple2_do_loader/apple2_do_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

sdd_speccydos_loader.o : $(BASEDIR)/loaders/sdd_speccydos_loader/sdd_speccydos_loader.c $(BASEDIR)/loaders/sdd_speccydos_loader/sdd_speccydos_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

sdd_speccydos_writer.o : $(BASEDIR)/loaders/sdd_speccydos_loader/sdd_speccydos_writer.c $(BASEDIR)/loaders/sdd_speccydos_loader/sdd_speccydos_writer.h
	$(CC) -o $@ -c $< $(CFLAGS)

adf_hd.o : $(BASEDIR)/thirdpartylibs/adflib/Lib/adf_hd.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_disk.o : $(BASEDIR)/thirdpartylibs/adflib/Lib/adf_disk.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_raw.o : $(BASEDIR)/thirdpartylibs/adflib/Lib/adf_raw.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_bitm.o: $(BASEDIR)/thirdpartylibs/adflib/Lib/adf_bitm.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_dump.o: $(BASEDIR)/thirdpartylibs/adflib/Lib/adf_dump.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_util.o  : $(BASEDIR)/thirdpartylibs/adflib/Lib/adf_util.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_env.o :  $(BASEDIR)/thirdpartylibs/adflib/Lib/adf_env.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_nativ.o :$(BASEDIR)/thirdpartylibs/adflib/Lib/adf_nativ.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_dir.o : $(BASEDIR)/thirdpartylibs/adflib/Lib/adf_dir.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_file.o : $(BASEDIR)/thirdpartylibs/adflib/Lib/adf_file.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_cache.o : $(BASEDIR)/thirdpartylibs/adflib/Lib/adf_cache.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_link.o : $(BASEDIR)/thirdpartylibs/adflib/Lib/adf_link.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_salv.o : $(BASEDIR)/thirdpartylibs/adflib/Lib/adf_salv.c
	$(CC) -o $@ -c $< $(CFLAGS)


getbits.o : $(BASEDIR)/thirdpartylibs/xdms/xdms-1.3.2/src/getbits.c
	$(CC) -o $@ -c $< $(CFLAGS)
crc_csum.o : $(BASEDIR)/thirdpartylibs/xdms/xdms-1.3.2/src/crc_csum.c
	$(CC) -o $@ -c $< $(CFLAGS)
maketbl.o : $(BASEDIR)/thirdpartylibs/xdms/xdms-1.3.2/src/maketbl.c
	$(CC) -o $@ -c $< $(CFLAGS)
pfile.o : $(BASEDIR)/thirdpartylibs/xdms/xdms-1.3.2/src/pfile.c
	$(CC) -o $@ -c $< $(CFLAGS)
tables.o : $(BASEDIR)/thirdpartylibs/xdms/xdms-1.3.2/src/tables.c
	$(CC) -o $@ -c $< $(CFLAGS)
u_deep.o : $(BASEDIR)/thirdpartylibs/xdms/xdms-1.3.2/src/u_deep.c
	$(CC) -o $@ -c $< $(CFLAGS)
u_heavy.o : $(BASEDIR)/thirdpartylibs/xdms/xdms-1.3.2/src/u_heavy.c
	$(CC) -o $@ -c $< $(CFLAGS)
u_init.o : $(BASEDIR)/thirdpartylibs/xdms/xdms-1.3.2/src/u_init.c
	$(CC) -o $@ -c $< $(CFLAGS)
u_medium.o : $(BASEDIR)/thirdpartylibs/xdms/xdms-1.3.2/src/u_medium.c
	$(CC) -o $@ -c $< $(CFLAGS)
u_quick.o : $(BASEDIR)/thirdpartylibs/xdms/xdms-1.3.2/src/u_quick.c
	$(CC) -o $@ -c $< $(CFLAGS)
u_rle.o :   $(BASEDIR)/thirdpartylibs/xdms/xdms-1.3.2/src/u_rle.c
	$(CC) -o $@ -c $< $(CFLAGS)


adler32.o : $(BASEDIR)/thirdpartylibs/zlib/adler32.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ
compress.o : $(BASEDIR)/thirdpartylibs/zlib/compress.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ
crc32.o : $(BASEDIR)/thirdpartylibs/zlib/crc32.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ
deflate.o : $(BASEDIR)/thirdpartylibs/zlib/deflate.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ
gzlib.o : $(BASEDIR)/thirdpartylibs/zlib/gzlib.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ
gzread.o : $(BASEDIR)/thirdpartylibs/zlib/gzread.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ
gzwrite.o : $(BASEDIR)/thirdpartylibs/zlib/gzwrite.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ
gzclose.o : $(BASEDIR)/thirdpartylibs/zlib/gzclose.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ

infback.o : $(BASEDIR)/thirdpartylibs/zlib/infback.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ
inffast.o : $(BASEDIR)/thirdpartylibs/zlib/inffast.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ
inflate.o : $(BASEDIR)/thirdpartylibs/zlib/inflate.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ
inftrees.o : $(BASEDIR)/thirdpartylibs/zlib/inftrees.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ
trees.o : $(BASEDIR)/thirdpartylibs/zlib/trees.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ
uncompr.o : $(BASEDIR)/thirdpartylibs/zlib/uncompr.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ
zutil.o : $(BASEDIR)/thirdpartylibs/zlib/zutil.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNO_VIZ

ioapi.o : $(BASEDIR)/thirdpartylibs/zlib/contrib/minizip/ioapi.c
	$(CC) -o $@ -c $< $(CFLAGS) -DNOLONGFILESUPPORT -DUSE_FILE32API -DIOAPI_NO_64
mztools.o : $(BASEDIR)/thirdpartylibs/zlib/contrib/minizip/mztools.c
	$(CC) -o $@ -c $< $(CFLAGS) -DUSE_FILE32API -DIOAPI_NO_64
unzip.o : $(BASEDIR)/thirdpartylibs/zlib/contrib/minizip/unzip.c
	$(CC) -o $@ -c $< $(CFLAGS) -DUSE_FILE32API -DIOAPI_NO_64
zip.o : $(BASEDIR)/thirdpartylibs/zlib/contrib/minizip/zip.c
	$(CC) -o $@ -c $< $(CFLAGS) -DUSE_FILE32API -DIOAPI_NO_64

libsap.o : $(BASEDIR)/thirdpartylibs/libsap/libsap.c   $(BASEDIR)/thirdpartylibs/libsap/libsap.h
	$(CC) -o $@ -c $< $(CFLAGS)

xml_disk.o : ../sources/xml_disk/xml_disk.c
	$(CC) -o $@ -c $< $(CFLAGS)  -I ../sources/thirdpartylibs/expat/expat-2.1.0/lib

lzw.o : ../sources/xml_disk/packer/lzw.c
	$(CC) -o $@ -c $< $(CFLAGS)

pack.o : ../sources/xml_disk/packer/pack.c
	$(CC) -o $@ -c $< $(CFLAGS)

rle.o : ../sources/xml_disk/packer/rle.c
	$(CC) -o $@ -c $< $(CFLAGS)

xmlparse.o : ../sources/thirdpartylibs/expat/expat-2.1.0/lib/xmlparse.c
	$(CC) -o $@ -c $< $(CFLAGS) -DHAVE_MEMMOVE

xmlrole.o : ../sources/thirdpartylibs/expat/expat-2.1.0/lib/xmlrole.c
	$(CC) -o $@ -c $< $(CFLAGS) -DHAVE_MEMMOVE

xmltok.o : ../sources/thirdpartylibs/expat/expat-2.1.0/lib/xmltok.c
	$(CC) -o $@ -c $< $(CFLAGS) -DHAVE_MEMMOVE


ftdi_d2xxmode.o : ftdi.c ftdi.h ftd2xx.h WinTypes.h
	$(CC) -o $@ -c $< $(CFLAGS)

ftdi_libftdimode.o : ftdi.c ftdi.h ftd2xx.h WinTypes.h
	$(CC) -o $@ -c $< $(CFLAGS) -DFTDILIB

fat_access.o : $(BASEDIR)/thirdpartylibs/FATIOlib/fat_access.c
	$(CC) -o $@ -c $< $(CFLAGS)

fat_cache.o : $(BASEDIR)/thirdpartylibs/FATIOlib/fat_cache.c
	$(CC) -o $@ -c $< $(CFLAGS)

fat_filelib.o : $(BASEDIR)/thirdpartylibs/FATIOlib/fat_filelib.c
	$(CC) -o $@ -c $< $(CFLAGS)

fat_format.o : $(BASEDIR)/thirdpartylibs/FATIOlib/fat_format.c
	$(CC) -o $@ -c $< $(CFLAGS)

fat_misc.o : $(BASEDIR)/thirdpartylibs/FATIOlib/fat_misc.c
	$(CC) -o $@ -c $< $(CFLAGS)

fat_string.o : $(BASEDIR)/thirdpartylibs/FATIOlib/fat_string.c
	$(CC) -o $@ -c $< $(CFLAGS)

fat_table.o : $(BASEDIR)/thirdpartylibs/FATIOlib/fat_table.c
	$(CC) -o $@ -c $< $(CFLAGS)

fat_write.o : $(BASEDIR)/thirdpartylibs/FATIOlib/fat_write.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

.PHONY: clean mrproper

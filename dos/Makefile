#CC=i386-pc-linux-gcc
CC=gcc
CFLAGS=-O3 -I . -I ../common/ -I ../common/includes -I ../common/ftdi_floppyemulator
LDFLAGS=-lc -lm  
EXEC=hxcfloppyemulator

ADFLIBOBJ= adf_hd.o adf_disk.o adf_raw.o adf_bitm.o adf_dump.o\
        adf_util.o adf_env.o adf_nativ.o adf_dir.o adf_file.o adf_cache.o \
        adf_link.o adf_salv.o

XDMSLIBOBJ= getbits.o crc_csum.o maketbl.o pfile.o getbits.o tables.o u_deep.o u_heavy.o u_init.o u_medium.o u_quick.o u_rle.o

ZLIBOBJ=adler32.o compress.o crc32.o deflate.o gzio.o infback.o inffast.o inflate.o inftrees.o trees.o uncompr.o zutil.o

DISKLOADERPLUGINS=copyqm_loader.o crctable.o adf_loader.o adz_loader.o amigadosfs_loader.o vfile.o cpcdsk_loader.o msa_loader.o msx_loader.o smc_loader.o st_loader.o stx_loader.o img_loader.o dms_loader.o mfm_loader.o oricdsk_loader.o fat12floppy_loader.o fat12.o

all: $(EXEC)

hxcfloppyemulator: hxcfloppyemu.o floppy_loader.o dos_api.o amiga_track.o crc.o iso_ibm_track.o  mfm_file_writer.o  $(ADFLIBOBJ) $(DISKLOADERPLUGINS) $(XDMSLIBOBJ) $(ZLIBOBJ)
	$(CC) -o $@    $^ $(LDFLAGS)

hxcfloppyemu.o: hxcfloppyemu.c
	$(CC) -o $@ -c $< $(CFLAGS)

floppy_loader.o: ../common/floppy_loader.c
	$(CC) -o $@ -c $< $(CFLAGS)

dos_api.o: dos_api.c
	$(CC) -o $@ -c $< $(CFLAGS)


variablebitrate.o : ../common/usb_floppyemulator/variablebitrate.c
	$(CC) -o $@ -c $< $(CFLAGS)

mfm_file_writer.o : ../common/mfm_file_writer.c ../common/mfm_file_writer.h
	$(CC) -o $@ -c $< $(CFLAGS)

amiga_track.o: ../common/plugins/common/amiga_track.c ../common/plugins/common/amiga_track.h
	$(CC) -o $@ -c $< $(CFLAGS)

crc.o: ../common/plugins/common/crc.c ../common/plugins/common/crc.h
	$(CC) -o $@ -c $< $(CFLAGS)

iso_ibm_track.o : ../common/plugins/common/iso_ibm_track.c ../common/plugins/common/iso_ibm_track.c
	$(CC) -o $@ -c $< $(CFLAGS)


copyqm_loader.o : ../common/plugins/copyqm_loader/copyqm_loader.c  ../common/plugins/copyqm_loader/crctable.c ../common/plugins/copyqm_loader/copyqm_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)
crctable.o : ../common/plugins/copyqm_loader/crctable.c   ../common/plugins/copyqm_loader/crctable.h
	$(CC) -o $@ -c $< $(CFLAGS)

adf_loader.o : ../common/plugins/adf_loader/adf_loader.c ../common/plugins/adf_loader/adf_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

cpcdsk_loader.o : ../common/plugins/cpcdsk_loader/cpcdsk_loader.c ../common/plugins/cpcdsk_loader/cpcdsk_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

msa_loader.o : ../common/plugins/msa_loader/msa_loader.c ../common/plugins/msa_loader/msa_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

msx_loader.o : ../common/plugins/msx_loader/msx_loader.c ../common/plugins/msx_loader/msx_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

st_loader.o : ../common/plugins/st_loader/st_loader.c ../common/plugins/st_loader/st_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

stx_loader.o : ../common/plugins/stx_loader/stx_loader.c ../common/plugins/stx_loader/stx_loader.h ../common/plugins/stx_loader/pasti_format.h
	$(CC) -o $@ -c $< $(CFLAGS)

img_loader.o : ../common/plugins/img_loader/img_loader.c ../common/plugins/img_loader/img_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

mfm_loader.o : ../common/plugins/mfm_loader/mfm_loader.c ../common/plugins/mfm_loader/mfm_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

smc_loader.o : ../common/plugins/smc_loader/snes_smc_loader.c ../common/plugins/smc_loader/snes_smc_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)


fat12floppy_loader.o : ../common/plugins/fat12floppy_loader/fat12floppy_loader.c ../common/plugins/fat12floppy_loader/fat12floppy_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)
fat12.o : ../common/plugins/fat12floppy_loader/fat12.c ../common/plugins/fat12floppy_loader/fat12.h
	$(CC) -o $@ -c $< $(CFLAGS)


oricdsk_loader.o : ../common/plugins/oricdsk_loader/oricdsk_loader.c ../common/plugins/oricdsk_loader/oricdsk_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)


amigadosfs_loader.o : ../common/plugins/amigadosfs_loader/amigadosfs_loader.c ../common/plugins/amigadosfs_loader/amigadosfs_loader.h
	$(CC) -o $@ -c $< $(CFLAGS)

dms_loader.o : ../common/plugins/dms_loader/dms_loader.c ../common/plugins/dms_loader/dms_loader.h 
	$(CC) -o $@ -c $< $(CFLAGS)

vfile.o : ../common/plugins/dms_loader/vfile.c ../common/plugins/dms_loader/vfile.h 
	$(CC) -o $@ -c $< $(CFLAGS)

adz_loader.o : ../common/plugins/adz_loader/adz_loader.c ../common/plugins/adz_loader/adz_loader.h 
	$(CC) -o $@ -c $< $(CFLAGS)

adf_hd.o : ../common/plugins/amigadosfs_loader/adflib/Lib/adf_hd.c 
	$(CC) -o $@ -c $< $(CFLAGS)
adf_disk.o : ../common/plugins/amigadosfs_loader/adflib/Lib/adf_disk.c 
	$(CC) -o $@ -c $< $(CFLAGS)
adf_raw.o : ../common/plugins/amigadosfs_loader/adflib/Lib/adf_raw.c 
	$(CC) -o $@ -c $< $(CFLAGS)
adf_bitm.o: ../common/plugins/amigadosfs_loader/adflib/Lib/adf_bitm.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_dump.o: ../common/plugins/amigadosfs_loader/adflib/Lib/adf_dump.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_util.o  : ../common/plugins/amigadosfs_loader/adflib/Lib/adf_util.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_env.o :  ../common/plugins/amigadosfs_loader/adflib/Lib/adf_env.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_nativ.o :../common/plugins/amigadosfs_loader/adflib/Lib/adf_nativ.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_dir.o : ../common/plugins/amigadosfs_loader/adflib/Lib/adf_dir.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_file.o : ../common/plugins/amigadosfs_loader/adflib/Lib/adf_file.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_cache.o : ../common/plugins/amigadosfs_loader/adflib/Lib/adf_cache.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_link.o : ../common/plugins/amigadosfs_loader/adflib/Lib/adf_link.c
	$(CC) -o $@ -c $< $(CFLAGS)
adf_salv.o : ../common/plugins/amigadosfs_loader/adflib/Lib/adf_salv.c
	$(CC) -o $@ -c $< $(CFLAGS)


getbits.o : ../common/plugins/dms_loader/xdms-1.3.2/src/getbits.c
	$(CC) -o $@ -c $< $(CFLAGS)
crc_csum.o : ../common/plugins/dms_loader/xdms-1.3.2/src/crc_csum.c
	$(CC) -o $@ -c $< $(CFLAGS)
maketbl.o : ../common/plugins/dms_loader/xdms-1.3.2/src/maketbl.c
	$(CC) -o $@ -c $< $(CFLAGS)
pfile.o : ../common/plugins/dms_loader/xdms-1.3.2/src/pfile.c
	$(CC) -o $@ -c $< $(CFLAGS)
tables.o : ../common/plugins/dms_loader/xdms-1.3.2/src/tables.c
	$(CC) -o $@ -c $< $(CFLAGS)
u_deep.o : ../common/plugins/dms_loader/xdms-1.3.2/src/u_deep.c
	$(CC) -o $@ -c $< $(CFLAGS)
u_heavy.o : ../common/plugins/dms_loader/xdms-1.3.2/src/u_heavy.c
	$(CC) -o $@ -c $< $(CFLAGS)
u_init.o : ../common/plugins/dms_loader/xdms-1.3.2/src/u_init.c
	$(CC) -o $@ -c $< $(CFLAGS)
u_medium.o : ../common/plugins/dms_loader/xdms-1.3.2/src/u_medium.c
	$(CC) -o $@ -c $< $(CFLAGS)
u_quick.o : ../common/plugins/dms_loader/xdms-1.3.2/src/u_quick.c
	$(CC) -o $@ -c $< $(CFLAGS)
u_rle.o : ../common/plugins/dms_loader/xdms-1.3.2/src/u_rle.c
	$(CC) -o $@ -c $< $(CFLAGS)


adler32.o : ../common/plugins/adz_loader/zlib123/adler32.c
	$(CC) -o $@ -c $< $(CFLAGS)
compress.o : ../common/plugins/adz_loader/zlib123/compress.c
	$(CC) -o $@ -c $< $(CFLAGS)
crc32.o : ../common/plugins/adz_loader/zlib123/crc32.c
	$(CC) -o $@ -c $< $(CFLAGS)
deflate.o : ../common/plugins/adz_loader/zlib123/deflate.c
	$(CC) -o $@ -c $< $(CFLAGS)
gzio.o : ../common/plugins/adz_loader/zlib123/gzio.c
	$(CC) -o $@ -c $< $(CFLAGS)
infback.o : ../common/plugins/adz_loader/zlib123/infback.c
	$(CC) -o $@ -c $< $(CFLAGS)
inffast.o : ../common/plugins/adz_loader/zlib123/inffast.c
	$(CC) -o $@ -c $< $(CFLAGS)
inflate.o : ../common/plugins/adz_loader/zlib123/inflate.c
	$(CC) -o $@ -c $< $(CFLAGS)
inftrees.o : ../common/plugins/adz_loader/zlib123/inftrees.c
	$(CC) -o $@ -c $< $(CFLAGS)
trees.o : ../common/plugins/adz_loader/zlib123/trees.c
	$(CC) -o $@ -c $< $(CFLAGS)
uncompr.o : ../common/plugins/adz_loader/zlib123/uncompr.c
	$(CC) -o $@ -c $< $(CFLAGS)
zutil.o : ../common/plugins/adz_loader/zlib123/zutil.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	del *.o

mrproper: clean
	del *.o

.PHONY: clean mrproper

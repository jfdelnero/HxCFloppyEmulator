
export DOWNLOADURL=https://www.fltk.org/pub/fltk/1.3.6/fltk-1.3.6-source.tar.gz
export ARCHIVENAMEBASE=fltk-1.3.6-source
export PATCHFILE=fltk-1.3.6.patch
export FOLDERNAME=fltk-1.3.6

if [ ! -d fltk-1.x.x ]; then
	wget $DOWNLOADURL -nc || exit 1
	tar -xzf ${ARCHIVENAMEBASE}.tar.gz
	mv ${FOLDERNAME} fltk-1.x.x
	patch -i ../${PATCHFILE} -p1 -d fltk-1.x.x
fi

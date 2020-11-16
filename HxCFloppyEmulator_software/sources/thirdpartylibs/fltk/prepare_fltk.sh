#export DOWNLOADURL=https://www.fltk.org/pub/fltk/1.3.5/fltk-1.3.5-source.tar.gz
#export ARCHIVENAMEBASE=fltk-1.3.5-source
#export PATCHFILE=fltk-1.3.5-vc6.patch
#export FOLDERNAME=fltk-1.3.5

export DOWNLOADURL=https://www.fltk.org/pub/fltk/snapshots/fltk-1.4.x-20201113-a4bacf83.tar.gz
export ARCHIVENAMEBASE=fltk-1.4.x-20201113-a4bacf83
export PATCHFILE=fltk-1.4.x-vc6.patch
export FOLDERNAME=fltk-1.4.x-20201113-a4bacf83

if [ ! -d fltk-1.x.x ]; then
	wget $DOWNLOADURL -nc || exit 1
	tar -xzf ${ARCHIVENAMEBASE}.tar.gz
	mv ${FOLDERNAME} fltk-1.x.x
	patch -i ../${PATCHFILE} -p1 -d fltk-1.x.x
fi


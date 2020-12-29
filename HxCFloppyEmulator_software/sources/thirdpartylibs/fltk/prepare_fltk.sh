
#if [[ "$OSTYPE" == "darwin"* ]]; then
#	export DOWNLOADURL=https://www.fltk.org/pub/fltk/snapshots/fltk-1.4.x-20201127-d7985607.tar.gz
#	export ARCHIVENAMEBASE=fltk-1.4.x-20201127-d7985607
#	export PATCHFILE=fltk-1.4.x-vc6.patch
#	export FOLDERNAME=fltk-1.4.x-20201127-d7985607
#else
#	export DOWNLOADURL=https://www.fltk.org/pub/fltk/1.3.5/fltk-1.3.5-source.tar.gz
#	export ARCHIVENAMEBASE=fltk-1.3.5-source
#	export PATCHFILE=fltk-1.3.5-vc6.patch
#	export FOLDERNAME=fltk-1.3.5
#fi

export DOWNLOADURL=https://www.fltk.org/pub/fltk/1.3.5/fltk-1.3.5-source.tar.gz
export ARCHIVENAMEBASE=fltk-1.3.5-source
export PATCHFILE=fltk-1.3.5-vc6.patch
export FOLDERNAME=fltk-1.3.5

if [ ! -d fltk-1.x.x ]; then
	wget $DOWNLOADURL -nc || exit 1
	tar -xzf ${ARCHIVENAMEBASE}.tar.gz
	mv ${FOLDERNAME} fltk-1.x.x
	patch -i ../${PATCHFILE} -p1 -d fltk-1.x.x
fi


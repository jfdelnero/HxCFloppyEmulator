#!/bin/bash

md5_check () {
	if [ "$(uname)" == "Darwin" ]; then
		export file_md5=`cat ${ARCHIVENAMEBASE}.tar.gz | md5`

		if [ "${file_md5}" != "${DOWNLOADHASH}" ]; then
			return 0;
		else
			return 1;
		fi;
	else
		export valid_md5=`echo ${DOWNLOADHASH} ${ARCHIVENAMEBASE}.tar.gz | md5sum -c - | grep ": OK" | wc -l`

		if [ "$valid_md5" -ne "1" ]; then
			return 0;
		else
			return 1;
		fi;
	fi;
}

if [ $OSTYPE == 'darwin'* ] || [ $1 = "1" ] ; then
export DOWNLOADURL=https://github.com/fltk/fltk/releases/download/release-1.4.1/fltk-1.4.1-source.tar.gz
export DOWNLOADHASH="203eed9e14a7bd6ff0373c0f3f32ef07"
export ARCHIVENAMEBASE=fltk-1.4.1-source
export PATCHFILE=
export FOLDERNAME=fltk-1.4.1
else

	if [ -z ${EMSDK+x} ]; then
		export DOWNLOADURL=https://github.com/fltk/fltk/releases/download/release-1.4.1/fltk-1.4.1-source.tar.gz
		export DOWNLOADHASH="203eed9e14a7bd6ff0373c0f3f32ef07"
		export ARCHIVENAMEBASE=fltk-1.4.1-source
		export PATCHFILE=fltk-1.4.x-vc6.patch
		export FOLDERNAME=fltk-1.4.1
	else
		# https://github.com/MoAlyousef/fltk_wasm32_emscripten/tree/emscripten
		# ("emscripten" branch)
		# + in Fl_Emscripten_Graphics_Driver.cxx
		# let idata = new ImageData(new Uint8ClampedArray(HEAPF64.buffer, $1, $2), $3, $4);
		# changed to :
		# let idata = new ImageData(new Uint8ClampedArray(HEAPF64.buffer, $1, $2).slice(), $3, $4);
		export DOWNLOADURL=https://hxc2001.com/vrac/fltk-1.4.0-wasm32.tar.gz
		export DOWNLOADHASH="16cba8128aaf4f24fcf748a08c764ea8"
		export ARCHIVENAMEBASE=fltk-1.4.0-wasm32
		export PATCHFILE=
		export FOLDERNAME=fltk-1.4.0-wasm32
	fi
fi

if [[ ! -d fltk-1.x.x ]]; then
	wget $DOWNLOADURL -nc || exit 1

	md5_check
	if [ "$?" -ne "1" ]; then
		echo "-----------------------------------------------------";
		echo "-- ERROR : Invalid FLTK sources archive md5sum !!! --";
		echo "--       Please check the download source !        --";
		echo "-----------------------------------------------------";
		exit 1;
	fi

	tar -xzf ${ARCHIVENAMEBASE}.tar.gz
	mv ${FOLDERNAME} fltk-1.x.x
	if [[ -n ${PATCHFILE} ]]; then
		patch -i ../${PATCHFILE} -p1 -d fltk-1.x.x || sleep 20
	fi
fi

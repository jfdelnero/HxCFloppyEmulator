#!/bin/bash

if [ $OSTYPE == 'darwin'* ] || [ $1 = "1" ] ; then
export DOWNLOADURL=https://www.fltk.org/pub/fltk/1.4.0/fltk-1.4.0-1-source.tar.gz
export ARCHIVENAMEBASE=fltk-1.4.0-1-source
export PATCHFILE=
export FOLDERNAME=fltk-1.4.0-1
else
export DOWNLOADURL=https://www.fltk.org/pub/fltk/1.4.0/fltk-1.4.0-1-source.tar.gz
export ARCHIVENAMEBASE=fltk-1.4.0-1-source
export PATCHFILE=fltk-1.4.x-vc6.patch
export FOLDERNAME=fltk-1.4.0-1
fi

if [[ ! -d fltk-1.x.x ]]; then
	wget $DOWNLOADURL -nc || exit 1
	tar -xzf ${ARCHIVENAMEBASE}.tar.gz
	mv ${FOLDERNAME} fltk-1.x.x
	if [[ -n ${PATCHFILE} ]]; then
		patch -i ../${PATCHFILE} -p1 -d fltk-1.x.x || sleep 20
	fi
fi

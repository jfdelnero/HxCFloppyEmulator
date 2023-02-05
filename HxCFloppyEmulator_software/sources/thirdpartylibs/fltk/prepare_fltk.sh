#!/bin/bash

if [[ $OSTYPE == 'darwin'* ]]; then
export DOWNLOADURL=https://www.fltk.org/pub/fltk/snapshots/fltk-1.4.x-20230203-8178d005.tar.gz
export ARCHIVENAMEBASE=fltk-1.4.x-20230203-8178d005
export PATCHFILE=
export FOLDERNAME=fltk-1.4.x-20230203-8178d005
else
export DOWNLOADURL=https://www.fltk.org/pub/fltk/1.3.8/fltk-1.3.8-source.tar.gz
export ARCHIVENAMEBASE=fltk-1.3.8-source
export PATCHFILE=fltk-1.3.8.patch
export FOLDERNAME=fltk-1.3.8
fi


if [[ ! -d fltk-1.x.x ]]; then
	wget $DOWNLOADURL -nc || exit 1
	tar -xzf ${ARCHIVENAMEBASE}.tar.gz
	mv ${FOLDERNAME} fltk-1.x.x
	if [[ -n ${PATCHFILE} ]]; then
		patch -i ../${PATCHFILE} -p1 -d fltk-1.x.x
	fi
fi

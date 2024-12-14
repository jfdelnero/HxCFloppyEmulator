#!/bin/bash

md5_check () {
	if [ "$(uname)" == "Darwin" ]; then
		export file_md5=`cat ${ARCHIVENAMEBASE}.tar.gz | md5`

		if [ "$file_md5" -ne "${DOWNLOADHASH}" ]; then
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
export DOWNLOADURL=https://github.com/fltk/fltk/releases/download/release-1.4.1/fltk-1.4.1-source.tar.gz
export DOWNLOADHASH="203eed9e14a7bd6ff0373c0f3f32ef07"
export ARCHIVENAMEBASE=fltk-1.4.1-source
export PATCHFILE=fltk-1.4.x-vc6.patch
export FOLDERNAME=fltk-1.4.1
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

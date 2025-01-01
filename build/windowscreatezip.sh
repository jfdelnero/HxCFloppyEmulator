mkdir HxCFloppyEmulator_Software
mkdir HxCFloppyEmulator_Software/Windows$1
cp ../hxcfloppyemulator_soft_release_notes.txt ./HxCFloppyEmulator_Software/ || exit 1
cp ../HxCFloppyEmulator_software/COPYING ./HxCFloppyEmulator_Software/ || exit 1
cp ../HxCFloppyEmulator_software/COPYING_FULL ./HxCFloppyEmulator_Software/ || exit 1
cp *.dll ./HxCFloppyEmulator_Software/Windows$1/ || exit 1
cp *.exe ./HxCFloppyEmulator_Software/Windows$1/ || exit 1
cp ../libhxcfe/sources/init.script ./HxCFloppyEmulator_Software/Windows$1/config.script || exit 1

zip -r HxCFloppyEmulator_Software_win$1.zip HxCFloppyEmulator_Software/

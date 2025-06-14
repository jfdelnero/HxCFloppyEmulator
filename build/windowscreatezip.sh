if [ "$2" = "HXCSOFT" ]; then
	mkdir HxCFloppyEmulator_Software
	mkdir HxCFloppyEmulator_Software/Windows$1
	cp ../hxcfloppyemulator_soft_release_notes.txt ./HxCFloppyEmulator_Software/ || exit 1
	cp ../HxCFloppyEmulator_software/COPYING ./HxCFloppyEmulator_Software/ || exit 1
	cp ../HxCFloppyEmulator_software/COPYING_FULL ./HxCFloppyEmulator_Software/ || exit 1
	cp *.dll ./HxCFloppyEmulator_Software/Windows$1/ || exit 1
	cp *.exe ./HxCFloppyEmulator_Software/Windows$1/ || exit 1
	cp ../libhxcfe/sources/init.script ./HxCFloppyEmulator_Software/Windows$1/config.script || exit 1

	zip -r HxCFloppyEmulator_Software_win$1.zip HxCFloppyEmulator_Software/
fi

if [ "$2" = "DOSDISKBROWSER" ]; then
	mkdir HxCFloppyEmulator_DosDiskBrowser
	mkdir HxCFloppyEmulator_DosDiskBrowser/Windows$1
	cp ../HxCFloppyEmulator_software/COPYING ./HxCFloppyEmulator_DosDiskBrowser/ || exit 1
	cp ../HxCFloppyEmulator_software/COPYING_FULL ./HxCFloppyEmulator_DosDiskBrowser/ || exit 1
	cp libhxcfe.dll ./HxCFloppyEmulator_DosDiskBrowser/Windows$1/ || exit 1
	cp DosDiskBrowser.exe ./HxCFloppyEmulator_DosDiskBrowser/Windows$1/ || exit 1

	zip -r HxCFloppyEmulator_DosDiskBrowser_win$1.zip HxCFloppyEmulator_DosDiskBrowser/
fi

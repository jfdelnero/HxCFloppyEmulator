mkdir HxCFloppyEmulator_DosDiskBrowser
mkdir HxCFloppyEmulator_DosDiskBrowser/Windows$1
cp ../HxCFloppyEmulator_software/COPYING ./HxCFloppyEmulator_DosDiskBrowser/ || exit 1
cp ../HxCFloppyEmulator_software/COPYING_FULL ./HxCFloppyEmulator_DosDiskBrowser/ || exit 1
cp libhxcfe.dll ./HxCFloppyEmulator_DosDiskBrowser/Windows$1/ || exit 1
cp DosDiskBrowser.exe ./HxCFloppyEmulator_DosDiskBrowser/Windows$1/ || exit 1

zip -r HxCFloppyEmulator_DosDiskBrowser_win$1.zip HxCFloppyEmulator_DosDiskBrowser/

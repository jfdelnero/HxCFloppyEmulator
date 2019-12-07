wget https://www.fltk.org/pub/fltk/1.3.5/fltk-1.3.5-source.tar.gz -nc || exit 1
rm -rf fltk-1.3.x/*
tar -xzf fltk-1.3.5-source.tar.gz
mv fltk-1.3.5 fltk-1.3.x
patch -i ../fltk-1.3.5-vc6.patch -p1 -d fltk-1.3.x

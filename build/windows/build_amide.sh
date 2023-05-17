set -e
set -x

export PATH=$PATH:/mingw64/bin
export AMIDE_PREFIX=/usr/local/amide

# Build and install XMedCon
wget https://prdownloads.sourceforge.net/xmedcon/xmedcon-0.23.0.zip
unzip xmedcon-0.23.0.zip
cd xmedcon-0.23.0
export XDG_DATA_DIRS=$XDG_DATA_DIRS:/mingw64/share/
export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:/mingw64/lib/pkgconfig:/mingw64/share/pkgconfig
./configure --disable-png --prefix=$AMIDE_PREFIX --enable-static=no
make install
cd ..

# Build and install VolPack
wget https://sourceforge.net/projects/amide/files/volpack/1.0c7/volpack-1.0c7.tgz
tar zxvf volpack-1.0c7.tgz
cd volpack-1.0c7
./configure --prefix=$AMIDE_PREFIX --enable-static=no
make
make install
cd ..

# Build and install Amide
cd amide-current
intltoolize
libtoolize
gtkdocize
gnome-doc-prepare
autoreconf --install --force --include=/mingw64/share/aclocal --include=$AMIDE_PREFIX/share/aclocal
export XMEDCON_CONFIG=$AMIDE_PREFIX/bin/xmedcon-config
export LDFLAGS=-L$AMIDE_PREFIX/lib
export CXXFLAGS=-I$AMIDE_PREFIX/include
./configure --prefix=$AMIDE_PREFIX --enable-amide-debug=no --enable-gtk-doc=no --disable-doc
export MSYS2_ARG_CONV_EXCL="/*"
make
make install
cd ..

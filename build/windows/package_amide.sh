set -e
set -x

export AMIDE_SOURCE_ROOT=$PWD/amide-current
export AMIDE_PACKAGE_ROOT=$PWD/package
export AMIDE_PREFIX=/usr/local/amide
export MINGW64_ROOT=/mingw64

mkdir -p $AMIDE_PACKAGE_ROOT
cd $AMIDE_PACKAGE_ROOT

cp $AMIDE_SOURCE_ROOT/etc/amide-*.iss ..

mkdir -p \
    bin \
    etc \
    lib/gtk-2.0/2.10.0 \
    share/pixmaps \
    share/man/man1 \
    share/applications

cp -r $MINGW64_ROOT/share/themes/MS-Windows/gtk-2.0 etc/
cp -r $MINGW64_ROOT/lib/gtk-2.0/2.10.0/engines lib/gtk-2.0/2.10.0
cp $AMIDE_SOURCE_ROOT/pixmaps/amide*logo.png share/pixmaps/
cp $AMIDE_SOURCE_ROOT/man/amide.1 share/man/man1/
cp $AMIDE_SOURCE_ROOT/etc/amide.desktop share/applications/
cp $AMIDE_SOURCE_ROOT/COPYING .

cp -t bin \
    $AMIDE_PREFIX/bin/amide.exe \
    $AMIDE_PREFIX/bin/libmdc*.dll \
    $AMIDE_PREFIX/bin/libvolpack*.dll \
    $MINGW64_ROOT/bin/avcodec*.dll \
    $MINGW64_ROOT/bin/avutil*.dll \
    $MINGW64_ROOT/bin/libaom.dll \
    $MINGW64_ROOT/bin/libart_lgpl_*.dll \
    $MINGW64_ROOT/bin/libatk-*.dll \
    $MINGW64_ROOT/bin/libbrotlicommon.dll \
    $MINGW64_ROOT/bin/libbrotlidec.dll \
    $MINGW64_ROOT/bin/libbz2-*.dll \
    $MINGW64_ROOT/bin/libcairo-*.dll \
    $MINGW64_ROOT/bin/libdatrie-*.dll \
    $MINGW64_ROOT/bin/libdav1d.dll \
    $MINGW64_ROOT/bin/libdcmdata.dll \
    $MINGW64_ROOT/bin/libdcmimgle.dll \
    $MINGW64_ROOT/bin/libdcmjpeg.dll \
    $MINGW64_ROOT/bin/libexpat-*.dll \
    $MINGW64_ROOT/bin/libffi-*.dll \
    $MINGW64_ROOT/bin/libfontconfig-*.dll \
    $MINGW64_ROOT/bin/libfreetype-*.dll \
    $MINGW64_ROOT/bin/libfribidi-*.dll \
    $MINGW64_ROOT/bin/libgailutil-*.dll \
    $MINGW64_ROOT/bin/libgcc_s_seh-*.dll \
    $MINGW64_ROOT/bin/libgdk_pixbuf-*.dll \
    $MINGW64_ROOT/bin/libgdk-win32-*.dll \
    $MINGW64_ROOT/bin/libgio-*.dll \
    $MINGW64_ROOT/bin/libglib-*.dll \
    $MINGW64_ROOT/bin/libgmodule-*.dll \
    $MINGW64_ROOT/bin/libgnomecanvas-*.dll \
    $MINGW64_ROOT/bin/libgobject-*.dll \
    $MINGW64_ROOT/bin/libgomp-*.dll \
    $MINGW64_ROOT/bin/libgraphite2.dll \
    $MINGW64_ROOT/bin/libgsl-*.dll \
    $MINGW64_ROOT/bin/libgslcblas-*.dll \
    $MINGW64_ROOT/bin/libgsm.dll \
    $MINGW64_ROOT/bin/libgtk-win32-*.dll \
    $MINGW64_ROOT/bin/libharfbuzz-*.dll \
    $MINGW64_ROOT/bin/libiconv-*.dll \
    $MINGW64_ROOT/bin/libicuuc72.dll \
    $MINGW64_ROOT/bin/libijg12.dll \
    $MINGW64_ROOT/bin/libijg16.dll \
    $MINGW64_ROOT/bin/libijg8.dll \
    $MINGW64_ROOT/bin/libintl-*.dll \
    $MINGW64_ROOT/bin/liblzma-*.dll \
    $MINGW64_ROOT/bin/liblzo2-*.dll \
    $MINGW64_ROOT/bin/libmfx-*.dll \
    $MINGW64_ROOT/bin/libmp3lame-*.dll \
    $MINGW64_ROOT/bin/liboflog.dll \
    $MINGW64_ROOT/bin/libofstd.dll \
    $MINGW64_ROOT/bin/libogg-*.dll \
    $MINGW64_ROOT/bin/libopencore-amrnb-*.dll \
    $MINGW64_ROOT/bin/libopencore-amrwb-*.dll \
    $MINGW64_ROOT/bin/libopenjp2-*.dll \
    $MINGW64_ROOT/bin/libopus-*.dll \
    $MINGW64_ROOT/bin/libpango-*.dll \
    $MINGW64_ROOT/bin/libpangocairo-*.dll \
    $MINGW64_ROOT/bin/libpangoft2-*.dll \
    $MINGW64_ROOT/bin/libpangowin32-*.dll \
    $MINGW64_ROOT/bin/libpcre2-*.dll \
    $MINGW64_ROOT/bin/libpixman-*.dll \
    $MINGW64_ROOT/bin/libpng16-*.dll \
    $MINGW64_ROOT/bin/librsvg-*.dll \
    $MINGW64_ROOT/bin/libsharpyuv-*.dll \
    $MINGW64_ROOT/bin/libsoxr.dll \
    $MINGW64_ROOT/bin/libspeex-*.dll \
    $MINGW64_ROOT/bin/libssp-*.dll \
    $MINGW64_ROOT/bin/libstdc++-*.dll \
    $MINGW64_ROOT/bin/libSvtAv1Enc.dll \
    $MINGW64_ROOT/bin/libthai-*.dll \
    $MINGW64_ROOT/bin/libtheoradec-*.dll \
    $MINGW64_ROOT/bin/libtheoraenc-*.dll \
    $MINGW64_ROOT/bin/libvorbis-*.dll \
    $MINGW64_ROOT/bin/libvorbisenc-*.dll \
    $MINGW64_ROOT/bin/libvpx-*.dll \
    $MINGW64_ROOT/bin/libwebp-*.dll \
    $MINGW64_ROOT/bin/libwebpmux-*.dll \
    $MINGW64_ROOT/bin/libwinpthread-*.dll \
    $MINGW64_ROOT/bin/libx264-*.dll \
    $MINGW64_ROOT/bin/libx265.dll \
    $MINGW64_ROOT/bin/libxml2-*.dll \
    $MINGW64_ROOT/bin/rav1e.dll \
    $MINGW64_ROOT/bin/swresample-*.dll \
    $MINGW64_ROOT/bin/vulkan-*.dll \
    $MINGW64_ROOT/bin/xvidcore.dll \
    $MINGW64_ROOT/bin/zlib1.dll \

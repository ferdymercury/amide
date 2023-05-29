FROM ubuntu:focal

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y \
    libgnomecanvas2-dev \
    libgconf2-dev \
    libmdc2-dev \
    libvolpack1-dev \
    libavcodec-dev \
    gtk-doc-tools \
    intltool \
    libxml2-dev \
    python-libxml2 \
    libgsl-dev \
    libdcmtk-dev \
    git \
    wget \
    python

RUN wget http://launchpadlibrarian.net/402991440/gnome-doc-utils_0.20.10-5_all.deb
RUN dpkg -i gnome-doc-utils_0.20.10-5_all.deb

RUN git clone --depth=1 https://github.com/ferdymercury/amide

WORKDIR /amide/amide-current

RUN intltoolize
RUN libtoolize
RUN gtkdocize
RUN gnome-doc-prepare
RUN autoreconf --install
RUN ./configure --prefix /opt/amide --enable-gnome-vfs=no --disable-scrollkeeper
RUN make
RUN mkdir -p /opt/amide
RUN make install

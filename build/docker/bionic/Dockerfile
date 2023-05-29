FROM ubuntu:bionic

RUN apt-get update && \
    apt-get install -y \
    libgnomecanvas2-dev \
    libgconf2-dev \
    libgnomevfs2-dev \
    gnome-doc-utils \
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
    rarian-compat

RUN git clone --depth=1 https://github.com/ferdymercury/amide

WORKDIR /amide/amide-current

RUN intltoolize
RUN libtoolize
RUN gtkdocize
RUN gnome-doc-prepare
RUN autoreconf --install
RUN ./configure --prefix /opt/amide
RUN make
RUN mkdir -p /opt/amide
RUN make install

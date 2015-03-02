Name: 		amide
Version: 	1.0.5
Release: 	2%{?dist}
Summary: 	Program for viewing and analyzing medical image data sets
License: 	GPLv2+
Group: 		Applications/Engineering
URL: 		http://amide.sourceforge.net
Source0: 	http://downloads.sourceforge.net/%{name}/%{name}-%{version}.tgz
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)
Packager: 	Andy Loening <loening at alum dot mit dot edu>

Requires:	xmedcon >= 0.10.0
Requires:	gsl
Requires:	volpack
Requires:	ffmpeg-libs >= 0.4.9
Requires:	dcmtk >= 3.6.0
Requires:       gtk2 >= 2.16
Requires:	gnome-vfs2
Requires:	libgnomecanvas

BuildRequires:  xmedcon-devel
BuildRequires:  volpack-devel 
BuildRequires:  libxml2-devel 
BuildRequires:  gnome-doc-utils
BuildRequires:  libgnomecanvas-devel 
BuildRequires:  ffmpeg-devel >= 0.4.9
BuildRequires:  gsl-devel
BuildRequires:  dcmtk-devel
BuildRequires:  perl-XML-Parser
BuildRequires:  glib2-devel
BuildRequires:  gtk2-devel >= 2.10
BuildRequires:	gnome-vfs2-devel

%description 

AMIDE is a tool for viewing and analyzing medical image data sets.
It's capabilities include the simultaneous handling of multiple data
sets imported from a variety of file formats, image fusion, 3D region
of interest drawing and analysis, volume rendering, and rigid body
alignments.


%prep
%setup -q

%build
%configure \
	   --enable-libecat=no \
	   --enable-amide-debug=no \
	   --disable-scrollkeeper
make

%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT

desktop-file-install --vendor gnome --delete-original                   \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications                         \
  --add-category X-Red-Hat-Extra                                        \
  $RPM_BUILD_ROOT%{_datadir}/applications/*


%clean
rm -rf $RPM_BUILD_ROOT

%post
update-desktop-database %{_datadir}/applications

%postun
update-desktop-database %{_datadir}/applications

%files
%defattr(-, root, root)
%doc AUTHORS COPYING ChangeLog NEWS README todo
%{_bindir}/*
%{_datadir}/pixmaps
%{_datadir}/gnome
%{_datadir}/omf
%{_datadir}/applications
%{_datadir}/locale
%{_mandir}/*



%changelog
* Fri Feb 24 2011 Andy Loening <loening at alum dot mit dot edu>
- cutout gtk-doc building and scrollkeeper
* Sun Dec 16 2007 Andy Loening <loening at alum dot mit dot edu>
- small tweak for new gnome-doc help files
* Tue Nov 05 2002 Andy Loening <loening at alum dot mit dot edu>
- get it to work with scrollkeeper
* Sun Dec 19 2000 Andy Loening <loening at alum dot mit dot edu>
- wrote this fool thing

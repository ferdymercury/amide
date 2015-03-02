Name: 		amide
Summary: 	amide is a program for viewing and analyzing medical image data sets
Version: 	0.8.19
Release: 	1
License: 	GPL
Group: 		Applications/Engineering
Source: 	%{name}-%{version}.tgz
URL: 		http://amide.sourceforge.net
BuildRoot: 	%{_tmppath}/%{name}-%{version}-root
Packager: 	Andy Loening <loening at alum dot mit dot edu>

Requires:	xmedcon >= 0.9.9.0
Requires:	gsl
Requires:	volpack
Requires:	libfame >= 0.9.1
Requires:	dcmtk >= 3.5.4

PreReq:		scrollkeeper >= 0.1.4

BuildRequires:  xmedcon-devel
BuildRequires:  volpack-devel 
BuildRequires:  libxml2-devel 
BuildRequires:  gtk-doc 
BuildRequires:  libgnomeui-devel 
BuildRequires:  libgnomecanvas-devel 
BuildRequires:  libfame-devel >= 0.9.1 
BuildRequires:  gsl-devel
BuildRequires:  dcmtk-devel

%description 
AMIDE is a tool for viewing and analyzing medical image data sets.
It's capabilities include the simultaneous handling of multiple data
sets imported from a variety of file formats, image fusion, 3D region
of interest drawing and analysis, volume rendering, and rigid body
alignments.


%prep
%setup -q

%build
%configure --enable-gtk-doc=yes --enable-libecat=no --enable-amide-debug=no
make

%install
rm -rf $RPM_BUILD_ROOT
%makeinstall

rm -rf $RPM_BUILD_ROOT/var/scrollkeeper
desktop-file-install --vendor gnome --delete-original                   \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications                         \
  --add-category X-Red-Hat-Extra                                        \
  $RPM_BUILD_ROOT%{_datadir}/applications/*

%clean
rm -rf $RPM_BUILD_ROOT

%post
update-desktop-database %{_datadir}/applications
scrollkeeper-update

%postun
update-desktop-database %{_datadir}/applications
scrollkeeper-update

%files
%defattr(-, root, root)
%doc AUTHORS COPYING ChangeLog NEWS README todo
%{_bindir}/*
%{_datadir}/pixmaps
%{_datadir}/gnome
%{_datadir}/omf
%{_datadir}/applications
%{_datadir}/gtk-doc
%{_datadir}/locale
%{_mandir}/*



%changelog
* Tue Nov 05 2002 Andy Loening <loening at alum dot mit dot edu>
- get it to work with scrollkeeper
* Sun Dec 19 2000 Andy Loening <loening at alum dot mit dot edu>
- wrote this fool thing

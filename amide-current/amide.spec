Name: 		amide
Summary: 	amide is a program for viewing and analyzing medical image data sets
Version: 	0.8.3
Release: 	1
License: 	GPL
Group: 		Applications/Engineering
Source: 	%{name}-%{version}.tgz
URL: 		http://amide.sourceforge.net
BuildRoot: 	%{_tmppath}/%{name}-%{version}-root
Packager: 	Andy Loening <loening at alum dot mit dot edu>

BuildRequires: 	xmedcon >= 0.9.4
BuildRequires:  volpack 
BuildRequires:  libxml2-devel 
BuildRequires:  gtk-doc 
BuildRequires:  libgnomeui-devel 
BuildRequires:  libgnomecanvas-devel 
BuildRequires:  libfame-devel
BuildRequires:  gsl-devel

%description 
AMIDE is a tool for viewing and analyzing medical image data sets.
It's capabilities include the simultaneous handling of multiple data
sets imported from a variety of file formats, image fusion, 3D region
of interest drawing and analysis, volume rendering, and rigid body
alignments.


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


%prep
%setup -n %{name}-%{version}
%build

%configure --enable-gtk-doc=yes --enable-libecat=no --enable-amide-debug=no
make

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install

./mkinstalldirs $RPM_BUILD_ROOT%{_datadir}/applications
desktop-file-install --vendor gnome --delete-original                   \
  --dir $RPM_BUILD_ROOT%{_datadir}/applications                         \
  --add-category X-Red-Hat-Extra                                        \
  $RPM_BUILD_ROOT%{_datadir}/applications/*

%clean
if [ -d $RPM_BUILD_ROOT ] ; then rm -rf $RPM_BUILD_ROOT; fi

%post
scrollkeeper-update

%postun
scrollkeeper-update


%changelog
* Tue Nov 05 2002 Andy Loening <loening at alum dot mit dot edu>
- get it to work with scrollkeeper
* Sun Dec 19 2000 Andy Loening <loening at alum dot mit dot edu>
- wrote this fool thing

%define name amide
%define prefix /usr
%define version 0.4.0
%define release 1

Summary: program for viewing and analyzing medical image data sets
Group: Applications/Engineering
Packager: Andy Loening <loening@ucla.edu>
Name: %{name}
Version: %{version}
Release: %{release}
Prefix: %{prefix}
Copyright: GPL
URL: http://amide.sourceforge.net
Source: %{name}-%{version}.tgz
Buildroot: /tmp/%{name}-%{version}
BuildRequires: libecat xmedcon volpack libxml2-devel

%description 
AMIDE is a tool for viewing and analyzing medical image data sets.

%prep
%setup -n %{name}-%{version}
%build
./configure --prefix=%{prefix}
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{prefix}/bin
mkdir -p $RPM_BUILD_ROOT/%{prefix}/share/gnome/help/amide/C
mkdir -p $RPM_BUILD_ROOT/%{prefix}/share/pixmaps
make prefix=$RPM_BUILD_ROOT/%{prefix} install

%clean
if [ -d $RPM_BUILD_ROOT ] ; then rm -rf $RPM_BUILD_ROOT; fi

%files
%defattr(-, root, root)
%{prefix}/bin/amide
%{prefix}/share/gnome/help/amide/*


%changelog
* Sun Dec 19 2000 Andy Loening <loening@ucla.edu>
- wrote this fool thing

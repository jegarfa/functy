Summary: Functy
Name: functy
Version: 0.30
Release: 1
Source0: %{name}-%{version}.tar.gz
License: MIT
Group: Productivity/Graphics/Visualization/Graph
BuildRoot: %{_builddir}/%{name}-root
%description
Functy is a 3D graph drawing package. The emphasis for the 
application is to allow Cartesian and spherical functions to be 
plotted and altered quickly and easily. This immediacy and the vivid 
results are intended to promote fun exploration of 3D functions.
%prep
%setup -q
%build
./configure
make
%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install
%clean
rm -rf $RPM_BUILD_ROOT
%files
%defattr(-,root,root)
/usr/local/bin/functy
/usr/local/share/functy/Functy.glade
%doc /usr/local/share/doc/functy/AUTHORS
%doc /usr/local/share/doc/functy/COPYING
%doc /usr/local/share/doc/functy/ChangeLog
%doc /usr/local/share/doc/functy/INSTALL
%doc /usr/local/share/doc/functy/NEWS
%doc /usr/local/share/doc/functy/README
%doc COPYING AUTHORS README NEWS INSTALL ChangeLog


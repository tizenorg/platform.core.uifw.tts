Name:       tts
Summary:    Text To Speech client library and daemon
Version:    0.1.1
Release:    1
Group:      libs
License:    Samsung
Source0:    %{name}-%{version}.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(mm-player)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-file)

BuildRequires:  cmake

%description
Text To Speech client library and daemon.


%package devel
Summary:    Text To Speech header files for TTS development
Group:      libdevel
Requires:   %{name} = %{version}-%{release}

%description devel
Text To Speech header files for TTS development.


%prep
%setup -q -n %{name}-%{version}


%build
cmake . -DCMAKE_INSTALL_PREFIX=/usr
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest tts-server.manifest
/etc/smack/accesses2.d/tts-server.rule
%defattr(-,root,root,-)
%{_libdir}/lib*.so
%{_libdir}/voice/tts/1.0/ttsd.conf
%{_bindir}/tts-daemon
/usr/share/license/*

%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/tts.pc
%{_libdir}/pkgconfig/tts-setting.pc
%{_includedir}/tts.h
%{_includedir}/tts_setting.h
%{_includedir}/ttsp.h

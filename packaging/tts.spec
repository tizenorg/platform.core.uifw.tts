Name:       tts
Summary:    Text To Speech client library and daemon
Version:    0.1.1
Release:    1
Group:      libs
License:    Samsung
Source0:    tts-0.1.1.tar.gz
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(mm-player)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(dnet)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(ecore-input)
BuildRequires:  pkgconfig(openssl)

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
%make_install




%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig



%files
%defattr(-,root,root,-)
%{_libdir}/lib*.so
%{_bindir}/tts-daemon


%files devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/tts.pc
%{_libdir}/pkgconfig/tts-setting.pc
%{_includedir}/tts.h
%{_includedir}/tts_setting.h
%{_includedir}/ttsp.h

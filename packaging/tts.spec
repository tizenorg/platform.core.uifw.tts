Name:       tts
Summary:    Text To Speech client library and daemon
Version:    0.1.60
Release:    1
Group:      libs
License:    Samsung
Source0:    %{name}-%{version}.tar.gz
Source1001:	%{name}.manifest
Source1002:	%{name}-devel.manifest
Requires(post): /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(mm-player)
BuildRequires:  pkgconfig(mm-common)
BuildRequires:  pkgconfig(mm-session)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(vconf-internal-keys)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(ecore-file)
BuildRequires:  pkgconfig(capi-system-runtime-info)

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
cp %{SOURCE1001} %{SOURCE1002} .


%build
%cmake .
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
%make_install

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
/etc/config/sysinfo-tts.xml
%defattr(-,root,root,-)
%{_libdir}/lib*.so
%{_libdir}/voice/tts/1.0/ttsd.conf
%{_bindir}/tts-daemon*
/usr/share/license/*

%files devel
%manifest %{name}-devel.manifest
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/tts.pc
%{_libdir}/pkgconfig/tts-setting.pc
%{_includedir}/tts.h
%{_includedir}/tts_setting.h
%{_includedir}/ttsp.h

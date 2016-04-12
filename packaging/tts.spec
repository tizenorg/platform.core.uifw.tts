Name:       tts
Summary:    Text To Speech client library and daemon
Version:    0.2.43
Release:    1
Group:      Graphics & UI Framework/Voice Framework
License:    Apache-2.0
Source0:    %{name}-%{version}.tar.gz
Source1001: %{name}.manifest
Source1002: %{name}-devel.manifest
Requires(post):   /sbin/ldconfig
Requires(postun): /sbin/ldconfig
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(capi-base-common)
BuildRequires:  pkgconfig(capi-media-audio-io)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(libtzplatform-config)
BuildRequires:  pkgconfig(libxml-2.0)
BuildRequires:  pkgconfig(vconf)


BuildRequires:  cmake

%description
Text To Speech client library and daemon.


%package devel
Summary:    Text To Speech header files for TTS development
Group:      libdevel
Requires:   %{name} = %{version}-%{release}

%package setting-devel
Summary:    Text To Speech setting header files for TTS development
Group:      libdevel
Requires:   %{name} = %{version}-%{release}

%package engine-devel
Summary:    Text To Speech engine header files for TTS development
Group:      libdevel
Requires:   %{name} = %{version}-%{release}

%description devel
Text To Speech header files for TTS development.

%description setting-devel
Text To Speech setting header files for TTS development.

%description engine-devel
Text To Speech engine header files for TTS development.


%prep
%setup -q -n %{name}-%{version}
cp %{SOURCE1001} %{SOURCE1002} .


%build
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"

export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"


cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DLIBDIR=%{_libdir} -DINCLUDEDIR=%{_includedir} \
        -DTZ_SYS_RO_SHARE=%TZ_SYS_RO_SHARE -DTZ_SYS_BIN=%TZ_SYS_BIN

make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{TZ_SYS_RO_SHARE}/license
install LICENSE %{buildroot}%{TZ_SYS_RO_SHARE}/license/%{name}

%make_install

%post 
/sbin/ldconfig

mkdir -p %{_libdir}/voice

mkdir -p %{TZ_SYS_RO_SHARE}/voice/test

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/lib*.so
%{_bindir}/tts-daemon*
%{TZ_SYS_RO_SHARE}/voice/tts/1.0/tts-config.xml
%{TZ_SYS_RO_SHARE}/dbus-1/services/org.tizen.voice*
%{TZ_SYS_RO_SHARE}/voice/test/tts-test
%{TZ_SYS_RO_SHARE}/license/%{name}
/etc/dbus-1/system.d/tts-server.conf

%files devel
%manifest %{name}-devel.manifest
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/tts.pc
%{_includedir}/tts.h

%files setting-devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/tts-setting.pc
%{_includedir}/tts_setting.h

%files engine-devel
%defattr(-,root,root,-)
%{_libdir}/pkgconfig/tts-engine.pc
%{_includedir}/ttsp.h

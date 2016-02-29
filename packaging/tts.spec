Name:       tts
Summary:    Text To Speech client library and daemon
Version:    0.2.42
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
BuildRequires:  pkgconfig(libprivilege-control)
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


cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix} -DLIBDIR=%{_libdir} -DINCLUDEDIR=%{_includedir}
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/share/license
install LICENSE %{buildroot}/usr/share/license/%{name}

%make_install

%post 
/sbin/ldconfig

mkdir -p %{_libdir}/voice

mkdir -p /usr/share/voice

mkdir -p /opt/usr/data/voice/tts/1.0/engine-info

chown 5000:5000 /opt/usr/data/voice
chown 5000:5000 /opt/usr/data/voice/tts
chown 5000:5000 /opt/usr/data/voice/tts/1.0
chown 5000:5000 /opt/usr/data/voice/tts/1.0/engine-info

%postun -p /sbin/ldconfig

%files
%manifest %{name}.manifest
%defattr(-,root,root,-)
%{_libdir}/lib*.so
%{_libdir}/voice/tts/1.0/tts-config.xml
%{_bindir}/tts-daemon*
/usr/share/dbus-1/system-services/org.tizen.voice*
/etc/dbus-1/system.d/tts-server.conf
/opt/usr/devel/bin/tts-test
/usr/share/license/%{name}

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

%define AppProgram indent
%define AppVersion 2.0
%define AppRelease 20210227
%define ActualProg c%{AppProgram}
# $Id: indent.spec,v 1.44 2021/02/27 16:33:39 tom Exp $
Summary: %{ActualProg} - format C program sources
Name: c%{AppProgram}
Version: %{AppVersion}
Release: %{AppRelease}
License: GPLv2
Group: Applications/Development
URL: ftp://ftp.invisible-island.net/%{AppProgram}
Source0: %{AppProgram}-%{AppVersion}-%{AppRelease}.tgz
Packager: Thomas Dickey <dickey@invisible-island.net>

%description
The `%{ActualProg}' program changes the appearance of a C program by
inserting or deleting whitespace.

This is a stable version of indent, used in most programs at
    https://invisible-island.net/

It has some feature enhancements required by those programs,
not found in other versions of indent.

%prep

%define debug_package %{nil}

%setup -q -n %{AppProgram}-%{AppVersion}-%{AppRelease}

%build

INSTALL_PROGRAM='${INSTALL}' \
	./configure \
		--target %{_target_platform} \
		--prefix=%{_prefix} \
		--bindir=%{_bindir} \
		--datadir=%{_datadir} \
		--infodir=%{_infodir} \
		--mandir=%{_mandir} \
		--program-prefix=c

make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

make install                    DESTDIR=$RPM_BUILD_ROOT

strip $RPM_BUILD_ROOT%{_bindir}/%{ActualProg}

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/%{ActualProg}
%{_bindir}/tdindent
%{_bindir}/*-indent
%{_mandir}/man1/%{ActualProg}.*
%{_mandir}/man1/tdindent.*
%{_mandir}/man1/*-indent.*
%{_datadir}/tdindent/*
%{_infodir}/%{ActualProg}.*

%changelog
# each patch should add its ChangeLog entries here

* Mon Oct 04 2010 Thomas Dickey
- initial version

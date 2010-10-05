Summary: indent - format C program sources
%define AppProgram indent
%define AppVersion 2.0
%define AppRelease 20101004
# $XTermId: indent.spec,v 1.1 2010/10/04 23:56:37 tom Exp $
Name: %{AppProgram}
Version: %{AppVersion}
Release: %{AppRelease}
License: GPLv2
Group: Applications/Development
URL: ftp://invisible-island.net/%{AppProgram}
Source0: %{AppProgram}-%{AppVersion}-%{AppRelease}.tgz
Packager: Thomas Dickey <dickey@invisible-island.net>

%description
The `indent' program changes the appearance of a C program by
inserting or deleting whitespace.

This is a stable version of indent, used in most programs at
    http://invisible-island.net/

It has some feature enhancements required by those programs,
not found in other versions of indent.

%prep

%setup -q -n %{AppProgram}-%{AppVersion}-%{AppRelease}

%build

INSTALL_PROGRAM='${INSTALL}' \
	./configure \
		--target %{_target_platform} \
		--prefix=%{_prefix} \
		--bindir=%{_bindir} \
		--infodir=%{_infodir} \
		--mandir=%{_mandir}

make

%install
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

make install                    DESTDIR=$RPM_BUILD_ROOT

strip $RPM_BUILD_ROOT%{_bindir}/%{AppProgram}

%clean
[ "$RPM_BUILD_ROOT" != "/" ] && rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%{_bindir}/%{AppProgram}
%{_infodir}/%{AppProgram}.*

%changelog
# each patch should add its ChangeLog entries here

* Mon Oct 04 2010 Thomas Dickey
- initial version

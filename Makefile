# Makefile for GNU indent
# Copyright (C) 1994 Joseph Arceneaux
#
# This file is part of GNU indent.
#
# GNU indent is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
# 
# GNU indent is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU indent; see the file COPYING.  If not, write to
# the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

prefix=/usr/local
exec_prefix=${prefix}

all: makefile doall

doall:
	@${MAKE} -f makefile all

indent: makefile doindent

doindent:
	@${MAKE} -f makefile indent

.DEFAULT:
	@if test \! -f makefile; then ${MAKE} makefile; fi
	@${MAKE} -f makefile $@

makefile: makefile.in Makefile
	@echo Running configure script to generate makefile
	@echo
	@sh configure --prefix=${prefix} --exec_prefix=${exec_prefix}

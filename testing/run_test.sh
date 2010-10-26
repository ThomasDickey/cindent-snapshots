#!/bin/sh
# $Id: run_test.sh,v 1.13 2010/10/25 22:27:25 tom Exp $
# vi:ts=4 sw=4
CODE=0
unset CDPATH
if test $# = 0
then
	case $0 in
	*/*)
		TOP=`echo "$0" |sed -e 's,/[^/]*$,,'`
		;;
	*)
		TOP=.
		;;
	esac
fi

SCRIPTS=`echo "$TOP/../scripts" | sed -e 's,[^/][^/]*/../,,'`
PATH=`cd "$SCRIPTS";pwd`:$PATH
export PATH

INDENT_DATA="$SCRIPTS"
export INDENT_DATA

for SRC in $TOP/*.in
do
	test -f "$SRC" || continue

	name=`basename $SRC .in`
	for OPT in $SCRIPTS/*-indent
	do
		test -f "$OPT" || continue

		opt=`basename $OPT | sed -e 's/-indent//'`

		TST=$name-$opt.c
		ERR=$name-$opt.err

		REF=$TOP/$name-$opt.ref
		MSG=$TOP/$name-$opt.msg

		rm -f $TST
		cp $SRC $TST

		sh -c "./$OPT -v $TST >$ERR 2>&1"
		rm -f *~

		if test ! -f $TST
		then
			echo "?? expected output $TST"
		elif test -f $REF
		then
			if cmp -s $REF $TST
			then
				echo "... ok $REF"
			else
				diff -u $REF $TST
				CODE=1
			fi
		else
			echo "... saving $REF"
			mv $TST $REF
		fi
		rm -f $TST

		if test ! -f $ERR
		then
			echo "?? expected messages $ERR"
		else
			len=`wc -l $ERR | sed -e 's/^[ ]*//' -e 's/ .*//'`
			if test ! -f $MSG && test "x$len" = x1
			then
				:		# only interested in multiline response
			elif test -f $MSG
			then
				if cmp -s $MSG $ERR
				then
					echo "... ok $MSG"
				else
					diff -u $MSG $ERR
					CODE=1
				fi
			else
				echo "... saving messages $MSG"
				mv $ERR $MSG
			fi
			rm -f $ERR
		fi
	done
done
exit $CODE

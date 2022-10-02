#!/bin/sh
# $Id: run_test.sh,v 1.17 2022/10/02 18:08:01 tom Exp $
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
PATH=`cd "$TOP"/.. && pwd`:`cd "$SCRIPTS" && pwd`:$PATH
export PATH

INDENT_DATA="$SCRIPTS"
export INDENT_DATA

for SRC in $TOP/case*.[chyl]
do
	test -f "$SRC" || continue
	case "$SRC" in
	*-*)
		continue
		;;
	esac

	type=`basename $SRC | sed -e 's/^[^.]*//'`
	name=`basename $SRC $type`
	for OPT in $SCRIPTS/*-indent
	do
		test -f "$OPT" || continue

		opt=`basename $OPT | sed -e 's/-indent//'`

		TST=$name-$opt$type
		ERR=$name-$opt.err

		REF=$TOP/$name-$opt.ref
		MSG=$TOP/$name-$opt.msg

		rm -f $TST $ERR
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
				rm -f $TST
			else
				diff -u $REF $TST
				case $REF in
				*-twm.*|*-xorg.*)
					echo "... expect some differences!"
					rm -f $TST
					;;
				*)
					CODE=1
					;;
				esac
			fi
		else
			echo "... saving $REF"
			mv $TST $REF
		fi

		if test ! -f $ERR
		then
			echo "?? expected messages $ERR"
		else
			len=`wc -l $ERR | sed -e 's/^[ ]*//' -e 's/ .*//'`
			if test ! -f $MSG && test "x$len" = x1
			then
				rm -f $ERR
				:		# only interested in multiline response
			elif test -f $MSG
			then
				if cmp -s $MSG $ERR
				then
					echo "... ok $MSG"
					rm -f $ERR
				else
					diff -u $MSG $ERR
					CODE=1
				fi
			else
				echo "... saving messages $MSG"
				mv $ERR $MSG
			fi
		fi
	done
done
exit $CODE

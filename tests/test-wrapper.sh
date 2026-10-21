#!/bin/bash

# set a few global variables that may be used by the test
basedir=$(cd $srcdir && pwd)
datadir=$(pwd)
bindir="$datadir/../src"

sbsign=$bindir/sbsign
sbverify=$bindir/sbverify
sbattach=$bindir/sbattach

key="$datadir/private-key.rsa"
cert="$datadir/public-cert.pem"
intkey="$datadir/int-key.ec"
intcert="$datadir/int-cert.pem"
cakey="$datadir/ca-key.ec"
cacert="$datadir/ca-cert.pem"

export basedir datadir bindir sbsign sbverify sbattach key cert intkey intcert cakey cacert

function sigtable_params() {
	local filename="$1"
	local pe_offset optional_offset magic directory_offset
	pe_offset=$(od -An -tu4 -j60 -N4 "$filename") || return 1
	optional_offset=$((pe_offset + 24))
	magic=$(od -An -tu2 -j"$optional_offset" -N2 "$filename") || return 1
	case $((magic)) in
		267) directory_offset=$((optional_offset + 128)) ;;
		523) directory_offset=$((optional_offset + 144)) ;;
		*) return 1 ;;
	esac
	od -An -tu4 -j"$directory_offset" -N8 "$filename"
}
export -f sigtable_params

# 'test' needs to be an absolute path, as we will cd to a temporary
# directory before running the test
test="$PWD/$1"
rc=0

image="$datadir/test.pecoff"
export image

# create the temporary directory...
tempdir=$(mktemp -d "${TMPDIR:-/tmp}/sbsigntools.XXXXXXXXXX")

# ... and run the test in it.
( cd "$tempdir" && "$BASH" -e "$test" )

if [ $? -ne 0 ]
then
	echo "test $(basename $test) failed"
	echo
	rc=1
fi

rm -rf "$tempdir"

exit $rc

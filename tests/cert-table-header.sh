#!/bin/bash -e

# Extract the signature from a file containing a signature table,
# and write to stdout
function extract_sig() {
	filename="$1"
	cert_table_header_size=8

	params=($(hexdump -n$cert_table_header_size \
				-e '/4 "%u " /2 "%04x " /2 "%04x\n"' \
				"$filename"))
	cert_size=${params[0]}
	cert_revision=${params[1]}
	cert_type=${params[2]}

	# check type & revision
	[ "$cert_revision" -eq '0200' ]
	[ "$cert_type" -eq '0002' ]

	dd if="$filename" bs=1 skip=$cert_table_header_size \
	       count=$(($cert_size - $cert_table_header_size)) 2>/dev/null
}

function repeat() {
	str=$1
	count=$2
	for (( i = 0; $i < $count; i++ ))
	do
		echo -n "$str"
	done
}

cert="test.cert"
signed="test.signed"

for i in {1..8}
do
	# generate a variable-length parameter for the certificate subject
	subj="/CN=$(repeat 'x' $i)"

	# create a temporary cert, and sign the image with it
	openssl req -x509 -sha256 -subj "$subj" -new -key "$key" -out "$cert"
	"$sbsign" --cert "$cert" --key "$key" --output "$signed" "$image"

	# extract the sigtable
	params=($(sigtable_params "$signed"))

	# split and convert to base-10
	sigtable_offset=$((${params[0]}))
	sigtable_size=$((${params[1]}))

	# check that we have a correctly-padded sigtable
	[ $(($sigtable_size % 8)) -eq 0 ]

	sigtable='test.sigtable'

	dd if="$signed" bs=1 skip=$sigtable_offset count=$sigtable_size \
	       of=$sigtable 2>/dev/null

	# extract sig, and feed to openssl's PKCS7 parser
	extract_sig "$sigtable" | openssl pkcs7 -inform DER -noout
done

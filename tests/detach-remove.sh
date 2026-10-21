#!/bin/bash -ex

signed="test.signed"
unsigned="test.unsigned"

"$sbsign" --cert "$cert" --key "$key" --output "$signed" "$image"
cp "$signed" "$unsigned"
"$sbattach" --remove "$unsigned"

# ensure that there is no security directory
params=($(sigtable_params "$unsigned"))
[ "${params[0]}" -eq 0 ]
[ "${params[1]}" -eq 0 ]

##
# somewhat tricky: i386 pecoff binaries can be too short, so we add padding
# when signing, so make sure the sizes match modulo the padding
##
# ensure that the unsigned file is the same size as our original binary
[ $(( ($(wc -c < "$image")+7)&~7)) -eq $(( ($(wc -c < "$unsigned")+7)&~7)) ]


#!/bin/bash

set -ex
set -o pipefail

rm -rfv smoke-test
mkdir -p smoke-test
test_dir="$PWD/smoke-test"

for example in examples/*; do
	pushd $example
		cmake .
		make
		binary="$PWD/$(ls -Art | grep -v -i make | tail -n 1)"
		echo binary = $binary
		if [[ -f cuckoo_clock1_x.wav ]]; then
			cp -prv cuckoo_clock1_x.wav $test_dir/
		fi
		pushd $test_dir
			SMOKE_TEST=1 $binary
			ls -alh
		popd
	popd
done

echo final results
pushd $test_dir
ls -alh
popd

md5_observed=$(ls -1 | sort | md5sum -)
md5_expected="5e2368d9d9ab871a88bab96656bca37b  -"

if [[ $md5_observed != $md5_expected ]]; then
    echo ERROR: Something in the output changed.
    exit 1
fi

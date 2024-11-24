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

echo Final results:

pushd $test_dir
  ls -alh

  md5_observed=$(ls -1 | sort | md5sum -)
  md5_expected="983feb3f676764439e2663918a957c15  -"

  if [[ $md5_observed != $md5_expected ]]; then
      echo ERROR: Something in the output changed.
      exit 1
  fi
popd

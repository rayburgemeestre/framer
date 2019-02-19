#!/bin/bash

docker run -it -v $PWD:/usr/local/src/framer build_framer:18.04 /bin/bash -c "$@"

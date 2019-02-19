#!/bin/bash

docker build "$@" -t build_framer:18.04 .

echo docker login
echo docker tag build_framer:18.04 rayburgemeestre/build_framer:18.04
echo docker push rayburgemeestre/build_framer:18.04


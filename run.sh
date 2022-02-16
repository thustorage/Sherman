#! /usr/bin/bash

if [ ! -d "build" ]; then
	mkdir build
fi

cp script/* build
cd build

cmake .. && make -j

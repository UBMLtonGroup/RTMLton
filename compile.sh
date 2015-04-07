#!/bin/bash
rm -rf build
make dirs     &&
make runtime  &&
make compiler &&
make basis-no-check &&
make script &&
make mlbpathmap &&
make constants 


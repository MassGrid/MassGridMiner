#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./libbase58/.libs/
./bfgminer -o stratum+tcp://mgd.vvpool.com:5630 -u M9A3gr2U1PWF54BAjcTFJiioRTG2tcCD2J -p x -S opencl:auto -D --debuglog -L pdebug9.log


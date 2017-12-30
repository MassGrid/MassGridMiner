#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./libbase58/.libs/
./bfgminer -o stratum+tcp://esmart.dorapool.com:4333 -u M9A3gr2U1PWF54BAjcTFJiioRTG2tcCD2J -p 123 -S opencl:auto -D --debuglog -L debug3.log


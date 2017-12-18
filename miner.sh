#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./libbase58/.libs/
./bfgminer -o http://127.0.0.1:17705 -u mlgbcoinrpc -p mlgbcoinpassword --generate-to M9A3gr2U1PWF54BAjcTFJiioRTG2tcCD2J -S opencl:auto -D --debuglog -L cccc.log


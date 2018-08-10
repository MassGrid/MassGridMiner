#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./libbase58/.libs/
./bfgminer -o http://127.0.0.1:9442 -u user -p pwd --generate-to mfuZbmtgahmzMiMqNZuFVNEbfjz74Z1mC4 -S opencl:auto -D --debuglog -L debug01.log


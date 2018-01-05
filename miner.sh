#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./libbase58/.libs/
./bfgminer -o http://127.0.0.1:9442 -u mlgbcoinrpc -p mlgbcoinpassword --generate-to M8Dqe8rzUL7B34F98wHKpjSBmt6KmBgcwn -S opencl:auto -D --debuglog -L debug7.log


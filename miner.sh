#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./libbase58/.libs/
./bfgminer -o http://127.0.0.1:9442 -u mlgbcoinrpc -p mlgbcoinpassword --generate-to MNANBR7oFAkRSEbHs5hfcbrsZ9YY6mUV8K -S opencl:auto -D --debuglog -L debug.log


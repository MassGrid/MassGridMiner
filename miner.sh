#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./libbase58/.libs/
<<<<<<< HEAD
./bfgminer -o http://192.168.2.146:17705 -u mlgbcoinrpc -p 45Ud7A61K2amoeMDqx5GJnmmrq5jX9ndrhiAEJQK5c83 --generate-to MFZi2ZDvUbBiK9MxmUQzPr5C7fKCCM9LWk -S opencl:auto -D -d 1 --debuglog -L debug.log
=======
./bfgminer -o http://127.0.0.1:9442 -u mlgbcoinrpc -p mlgbcoinpassword --generate-to MNANBR7oFAkRSEbHs5hfcbrsZ9YY6mUV8K -S opencl:auto -D --debuglog -L debug.log
>>>>>>> master


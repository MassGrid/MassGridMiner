#!/bin/bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./libbase58/.libs/
./bfgminer -o http://192.168.2.146:17705 -u mlgbcoinrpc -p 45Ud7A61K2amoeMDqx5GJnmmrq5jX9ndrhiAEJQK5c83 --generate-to MFZi2ZDvUbBiK9MxmUQzPr5C7fKCCM9LWk -S opencl:auto -D -d 1 --debuglog -L debug.log


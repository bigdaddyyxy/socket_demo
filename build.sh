#! /bin/bash
set -x

gcc -Wall -std=c17 server.c wrap.c -o server

gcc -Wall -std=c17 client.c wrap.c -o client


set +x

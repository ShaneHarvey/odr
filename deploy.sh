#!/bin/bash

SRC=$(ls *.c *.h Makefile)
scp $SRC minix:/home/courses/cse533/students/cse533-14/cse533/
ssh minix "cd cse533 && make clean && make debug && ./deploy_app ODR_cse533-14 client_cse533-14 server_cse533-14"

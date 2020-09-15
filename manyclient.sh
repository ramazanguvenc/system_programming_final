#!/bin/bash

for ((i = 0 ; i < 1000 ; i++)); do
  ./client -a 127.0.0.1 -p 8080 -s $(( ( RANDOM %  6301))) -d $(( ( RANDOM %  6301))) &
done
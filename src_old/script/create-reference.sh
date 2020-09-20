#!/bin/bash

nano-pos-tool -a ci-len $1 | grep "      [0-9]" | sed 's/\]//g' | sed 's/\[//g' | awk '{print $1" "$2" "$5}'

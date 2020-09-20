#!/bin/bash

find /mnt/rclsfserv005/MSMM_nano/user_data/ -name "$1*.csv" | while read p; do cat "$p"; done | grep "^/" | sed -s 's/,*x,*[0-9]*,[0-9]*,,,//' | while read x; do if [ `echo $x | awk -F"," '{print NF-1}'` -eq 1 ] ; then echo $x,-1,-1; else echo $x; fi ; done

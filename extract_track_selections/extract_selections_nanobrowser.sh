#!/bin/bash

find /mnt/rclsfserv005/MSMM_nano/user_data/ -name "$1*.csv" | while read p; do cat "$p" >>$1.csv; done

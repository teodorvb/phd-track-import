#!/bin/bash

env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-level-detection import-tracks/level-detection-real-verify.csv "Real tracks for verification of level detection." skip-negative

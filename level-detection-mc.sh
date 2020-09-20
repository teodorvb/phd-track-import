#!/bin/bash

env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-level-detection import-tracks/level-detection-mc.csv "Simulated tracks used for level detection, MC"

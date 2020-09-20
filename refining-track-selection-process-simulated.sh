#!/bin/bash

env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-level-detection import-tracks/refining-track-selection-process.csv "Simulated tracks for testing the track filter"

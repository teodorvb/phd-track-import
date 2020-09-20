#!/bin/bash

env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-classification import-tracks/smi-classify-simulated.csv "Simulated positive and negative tracks for testing smi-classify"

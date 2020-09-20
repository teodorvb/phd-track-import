#!/bin/bash

env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-classification import-tracks/smi-classify-manual-selection-01.csv "Manually selected tracks" &>log01 &
env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-classification import-tracks/smi-classify-manual-selection-02.csv "Manually selected tracks" &>log02 &
env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-classification import-tracks/smi-classify-manual-selection-03.csv "Manually selected tracks" &>log03 &
env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-classification import-tracks/smi-classify-manual-selection-04.csv "Manually selected tracks" &>log04 &
env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-classification import-tracks/smi-classify-manual-selection-05.csv "Manually selected tracks" &>log05 &
env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-classification import-tracks/smi-classify-manual-selection-06.csv "Manually selected tracks" &>log06 &
env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-classification import-tracks/smi-classify-manual-selection-07.csv "Manually selected tracks" &>log07 &
env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-classification import-tracks/smi-classify-manual-selection-08.csv "Manually selected tracks" &>log08 &
env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-classification import-tracks/smi-classify-manual-selection-09.csv "Manually selected tracks" &>log09 &
env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-classification import-tracks/smi-classify-manual-selection-10.csv "Manually selected tracks" &>log10 &

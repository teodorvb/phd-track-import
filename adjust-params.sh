#!/bin/bash

env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-to-csv import-tracks/adjust-params.csv "Simulated positive tracks used to find optimal values for fetaure and backgrodun intensity"

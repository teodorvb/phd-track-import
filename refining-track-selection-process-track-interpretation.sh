#!/bin/bash

env PGHOST=localhost PGPORT=5055 PGUSER=teodor PGPASSWORD=4HWZQ3y60gKKcTNp build/tools/smi-db-import-track-interpretation import-tracks/refining-track-selection-process-track-interpretation.csv "Tracks for estimating parameters of fluorophore model (FSM)."

#!/bin/bash
mlton -show-def-use DefUse.du -stop tc -prefer-abs-paths true mlton/mlton.mlb

#!/usr/bin/env bash
# CCITT TIFFs written by ImageMagick (libtiff): 64 x 32, left half white,
# right half black. Group 4 (T.6) and Group 3 (T.4, "Fax").
set -euo pipefail
cd "$(dirname "$0")"
convert -size 32x32 xc:white -size 32x32 xc:black +append -monochrome -compress Group4 ccitt_g4_libtiff.tif
convert -size 32x32 xc:white -size 32x32 xc:black +append -monochrome -compress Fax ccitt_g3_libtiff.tif

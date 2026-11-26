#!/usr/bin/env bash
# Regenerates the encrypted PDF fixtures with qpdf (independent of this
# library's own encryption) from pdfs/hello_world.pdf. Test material only.
#
#   user_owner_aes256.pdf  user "user", owner "owner", AES-256 (R6)
#   user_owner_rc4.pdf     user "user", owner "owner", RC4-128 (R3)
#   blank_user_aes128.pdf  no user (open) password, owner "owner", AES-128 (R4)
set -euo pipefail
cd "$(dirname "$0")"
src=../../../pdfs/hello_world.pdf
qpdf --encrypt --user-password=user --owner-password=owner --bits=256 -- \
    "$src" user_owner_aes256.pdf
qpdf --allow-weak-crypto --encrypt --user-password=user --owner-password=owner \
    --bits=128 --use-aes=n -- "$src" user_owner_rc4.pdf
qpdf --encrypt --user-password= --owner-password=owner --bits=128 --use-aes=y -- \
    "$src" blank_user_aes128.pdf

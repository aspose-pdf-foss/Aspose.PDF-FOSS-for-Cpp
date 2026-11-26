#!/usr/bin/env bash
# Regenerates the RSASSA-PSS CMS test fixtures (OpenSSL 3) with the signer key
# from signer_aes.p12 (password "test"), over content.bin:
#
#   openssl_cms_pss.der       SHA-256, MGF1-SHA-256, salt = digest length
#   openssl_cms_pss_max.der   SHA-384, MGF1-SHA-384, maximum salt length
#   openssl_cms_pss_sha1.der  SHA-1, salt 20: every RSASSA-PSS-params default
set -euo pipefail
cd "$(dirname "$0")"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
openssl pkcs12 -in signer_aes.p12 -passin pass:test -nocerts -nodes -out "$work/key.pem"
sign() {  # out, md, extra keyopts...
    local out=$1 md=$2; shift 2
    openssl cms -sign -binary -in content.bin -signer signer.pem -inkey "$work/key.pem" \
        -md "$md" -keyopt rsa_padding_mode:pss "$@" -outform DER -out "$out"
}
sign openssl_cms_pss.der sha256 -keyopt rsa_pss_saltlen:digest
sign openssl_cms_pss_max.der sha384 -keyopt rsa_pss_saltlen:max
sign openssl_cms_pss_sha1.der sha1 -keyopt rsa_pss_saltlen:20

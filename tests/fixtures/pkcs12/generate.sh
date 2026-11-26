#!/usr/bin/env bash
# Regenerates the PKCS#12 / CMS test fixtures (OpenSSL 3). Test material only:
# the keys are throwaway and the password is "test".
#
#   ca.pem             self-signed test CA (RSA-2048)
#   signer.pem         signer certificate issued by the CA (RSA-2048)
#   signer_aes.p12     key + signer + CA: PBES2 (PBKDF2-HMAC-SHA256, AES-256-CBC),
#                      MAC HMAC-SHA256 — the OpenSSL 3 default
#   signer_legacy.p12  the same: pbeWithSHAAnd3-KeyTripleDES-CBC for the key,
#                      pbeWithSHAAnd40BitRC2-CBC for the certificates, MAC SHA-1
#   content.bin        bytes signed by OpenSSL below
#   openssl_cms.der    detached CMS SignedData over content.bin (SHA-256)
set -euo pipefail
cd "$(dirname "$0")"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

openssl req -x509 -newkey rsa:2048 -nodes -days 36500 -subj "/CN=FOSS Test CA" \
    -keyout "$work/ca.key" -out ca.pem 2>/dev/null
openssl req -newkey rsa:2048 -nodes -subj "/CN=FOSS Test Signer/O=Aspose FOSS" \
    -keyout "$work/signer.key" -out "$work/signer.csr" 2>/dev/null
openssl x509 -req -in "$work/signer.csr" -CA ca.pem -CAkey "$work/ca.key" \
    -CAcreateserial -days 36500 -out signer.pem 2>/dev/null

openssl pkcs12 -export -inkey "$work/signer.key" -in signer.pem -certfile ca.pem \
    -passout pass:test -out signer_aes.p12
openssl pkcs12 -export -legacy -inkey "$work/signer.key" -in signer.pem \
    -certfile ca.pem -passout pass:test -out signer_legacy.p12

printf 'The quick brown fox jumps over the lazy dog.\n' > content.bin
openssl cms -sign -binary -in content.bin -signer signer.pem -inkey "$work/signer.key" \
    -certfile ca.pem -md sha256 -outform DER -out openssl_cms.der
rm -f ca.srl

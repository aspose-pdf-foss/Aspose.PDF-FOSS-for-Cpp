#!/usr/bin/env bash
# Regenerates the ECDSA PKCS#12 / CMS test fixtures (OpenSSL 3). Test material
# only: the keys are throwaway and the password is "test".
#
#   ec_ca.pem            self-signed test CA (P-256)
#   ec256.p12            P-256 signer + CA: PBES2 / AES-256-CBC, MAC SHA-256
#   ec384.p12            P-384 signer + CA: legacy 3DES / RC2-40, MAC SHA-1
#   ec521.p12            P-521 signer + CA: PBES2 / AES-256-CBC
#   openssl_cms_ec256.der  detached CMS over content.bin (P-256, SHA-256)
#   openssl_cms_ec384.der  detached CMS over content.bin (P-384, SHA-384)
set -euo pipefail
cd "$(dirname "$0")"
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:P-256 -nodes -days 36500 \
    -subj "/CN=FOSS Test EC CA" -keyout "$work/ca.key" -out ec_ca.pem 2>/dev/null

signer() {  # curve name, file stem
    openssl req -newkey ec -pkeyopt "ec_paramgen_curve:$1" -nodes \
        -subj "/CN=FOSS EC $1 Signer/O=Aspose FOSS" \
        -keyout "$work/$2.key" -out "$work/$2.csr" 2>/dev/null
    openssl x509 -req -in "$work/$2.csr" -CA ec_ca.pem -CAkey "$work/ca.key" \
        -CAcreateserial -days 36500 -out "$work/$2.pem" 2>/dev/null
}
signer P-256 ec256
signer P-384 ec384
signer P-521 ec521

openssl pkcs12 -export -inkey "$work/ec256.key" -in "$work/ec256.pem" -certfile ec_ca.pem \
    -passout pass:test -out ec256.p12
openssl pkcs12 -export -legacy -inkey "$work/ec384.key" -in "$work/ec384.pem" \
    -certfile ec_ca.pem -passout pass:test -out ec384.p12
openssl pkcs12 -export -inkey "$work/ec521.key" -in "$work/ec521.pem" -certfile ec_ca.pem \
    -passout pass:test -out ec521.p12

openssl cms -sign -binary -in content.bin -signer "$work/ec256.pem" \
    -inkey "$work/ec256.key" -md sha256 -outform DER -out openssl_cms_ec256.der
openssl cms -sign -binary -in content.bin -signer "$work/ec384.pem" \
    -inkey "$work/ec384.key" -md sha384 -outform DER -out openssl_cms_ec384.der
rm -f ec_ca.srl

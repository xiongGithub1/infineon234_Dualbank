#!/bin/bash
# extract_pub.sh - Extract RSA public key to C header (no Python needed)
# Usage: bash extract_pub.sh public.pem public_key.h

PUB_KEY="$1"
OUT_H="$2"

if [ -z "$PUB_KEY" ] || [ -z "$OUT_H" ]; then
    echo "Usage: bash extract_pub.sh public.pem public_key.h"
    exit 1
fi

if [ ! -f "$PUB_KEY" ]; then
    echo "Error: Public key not found: $PUB_KEY"
    exit 1
fi

# Extract modulus hex (remove "Modulus=" prefix, lowercase)
MOD_HEX=$(openssl rsa -pubin -in "$PUB_KEY" -modulus -noout 2>/dev/null | sed 's/Modulus=//' | tr '[:upper:]' '[:lower:]')

if [ -z "$MOD_HEX" ] || [ "${#MOD_HEX}" -ne 512 ]; then
    echo "Error: Failed to extract modulus from $PUB_KEY"
    exit 1
fi

{
    echo "/* Auto-generated public key header for RSA-2048 verification */"
    echo "/* Generated from $PUB_KEY */"
    echo "#ifndef CRYPTO_PUBLIC_KEY_H_"
    echo "#define CRYPTO_PUBLIC_KEY_H_"
    echo ""
    echo "#define RSA_MODULUS_LEN   (256u)"
    echo "#define RSA_EXPONENT_LEN  (4u)"
    echo ""
    echo "static const uint8 rsa_public_modulus[RSA_MODULUS_LEN] ="
    echo "{"

    i=0
    line=""
    while [ $i -lt 512 ]; do
        byte="0x${MOD_HEX:$i:2}"
        if [ $i -eq 0 ]; then
            line="    $byte"
        elif [ $i -eq 504 ]; then
            line="$line, $byte"
        else
            rem=$((i % 16))
            if [ $rem -eq 0 ]; then
                echo "    $line,"
                line="$byte"
            else
                line="$line, $byte"
            fi
        fi
        i=$((i + 2))
    done
    echo "    $line"

    echo "};"
    echo ""
    echo "static const uint8 rsa_public_exponent[RSA_EXPONENT_LEN] ="
    echo "{"
    echo "    0x00, 0x01, 0x00, 0x01"
    echo "};"
    echo ""
    echo "#endif /* CRYPTO_PUBLIC_KEY_H_ */"
    echo ""
} > "$OUT_H"

echo "Generated: $OUT_H"

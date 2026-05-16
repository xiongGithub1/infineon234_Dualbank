# extract_pub.ps1 - Extract RSA public key to C header (no Python needed)
# Usage: powershell -ExecutionPolicy Bypass -File extract_pub.ps1 -PubKey public.pem -Out public_key.h

param(
    [Parameter(Mandatory=$true)]
    [string]$PubKey,

    [Parameter(Mandatory=$true)]
    [string]$Out
)

if (-not (Test-Path $PubKey)) {
    Write-Error "Public key not found: $PubKey"
    exit 1
}

# Get modulus hex from OpenSSL
$modLine = & openssl rsa -pubin -in $PubKey -modulus -noout 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to parse public key with OpenSSL"
    exit 1
}

# Remove "Modulus=" prefix
$modHex = ($modLine -replace "Modulus=", "").ToLower()

# Build C array lines
$lines = @()
$lines += "/* Auto-generated public key header for RSA-2048 verification */"
$lines += "/* Generated from $PubKey */"
$lines += "#ifndef CRYPTO_PUBLIC_KEY_H_"
$lines += "#define CRYPTO_PUBLIC_KEY_H_"
$lines += ""
$lines += "#define RSA_MODULUS_LEN   (256u)"
$lines += "#define RSA_EXPONENT_LEN  (4u)"
$lines += ""
$lines += "static const uint8 rsa_public_modulus[RSA_MODULUS_LEN] ="
$lines += "{"

$bytes = @()
for ($i = 0; $i -lt $modHex.Length; $i += 2) {
    $bytes += "0x$($modHex.Substring($i, 2))"
}

# Format 8 bytes per line
for ($i = 0; $i -lt $bytes.Count; $i += 8) {
    $end = [Math]::Min($i + 8, $bytes.Count)
    $row = $bytes[$i..($end-1)] -join ", "
    if ($end -eq $bytes.Count) {
        $lines += "    $row"
    } else {
        $lines += "    $row,"
    }
}

$lines += "};"
$lines += ""
$lines += "static const uint8 rsa_public_exponent[RSA_EXPONENT_LEN] ="
$lines += "{"
$lines += "    0x00, 0x01, 0x00, 0x01"
$lines += "};"
$lines += ""
$lines += "#endif /* CRYPTO_PUBLIC_KEY_H_ */"
$lines += ""

$lines | Out-File -FilePath $Out -Encoding utf8
Write-Host "Generated: $Out"

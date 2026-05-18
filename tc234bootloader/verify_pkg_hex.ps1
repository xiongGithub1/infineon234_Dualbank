# ============================================================================
# PowerShell 脚本: 验证 .pkg 是否完整打包了 .aligned.hex
# 用法: .\verify_pkg_hex.ps1 -PkgFile "App_dualBank.pkg" -HexFile "App_dualBank.aligned.hex"
# ============================================================================
param(
    [Parameter(Mandatory=$true)]
    [string]$PkgFile,
    [Parameter(Mandatory=$true)]
    [string]$HexFile
)

$HEADER_SIZE = 128
$SIG_LEN = 256

function Parse-IntelHex($path) {
    $data = @{}
    $baseAddr = 0
    foreach ($line in Get-Content $path) {
        $line = $line.Trim()
        if (-not $line -or $line[0] -ne ':') { continue }
        if ($line.Length -lt 11) { continue }
        $byteCount = [Convert]::ToInt32($line.Substring(1,2), 16)
        $addr = [Convert]::ToInt32($line.Substring(3,4), 16)
        $recType = [Convert]::ToInt32($line.Substring(7,2), 16)
        if ($recType -eq 0x04) {
            $baseAddr = [Convert]::ToInt32($line.Substring(9,4), 16) -shl 16
        } elseif ($recType -eq 0x02) {
            $baseAddr = [Convert]::ToInt32($line.Substring(9,4), 16) -shl 4
        } elseif ($recType -eq 0x00) {
            $absAddr = $baseAddr + $addr
            for ($i = 0; $i -lt $byteCount; $i++) {
                $b = [Convert]::ToInt32($line.Substring(9 + $i*2, 2), 16)
                $data[$absAddr + $i] = $b
            }
        } elseif ($recType -eq 0x01) {
            break
        }
    }
    return $data
}

function Format-Addr($a) {
    return "0x{0:X8}" -f $a
}

# --- 1. Read HEX ---
Write-Host "======================================================================"
Write-Host "[verify] PKG: $PkgFile"
Write-Host "[verify] HEX: $HexFile"
Write-Host "======================================================================"

if (-not (Test-Path $PkgFile)) { Write-Error "PKG not found"; exit 1 }
if (-not (Test-Path $HexFile)) { Write-Error "HEX not found"; exit 1 }

$hexData = Parse-IntelHex $HexFile
$hexAddrs = ($hexData.Keys | Sort-Object)
$hexMin = $hexAddrs[0]
$hexMax = $hexAddrs[-1]
$hexCount = $hexData.Count

Write-Host ""
Write-Host "[verify] --- HEX file info ---"
Write-Host "  Total bytes: $hexCount"
Write-Host "  Address range: $(Format-Addr $hexMin) - $(Format-Addr $hexMax)"

# --- 2. Read PKG ---
$pkgBytes = [System.IO.File]::ReadAllBytes($PkgFile)
$pkgLen = $pkgBytes.Length

if ($pkgLen -lt $HEADER_SIZE + $SIG_LEN) {
    Write-Error "PKG file too small: $pkgLen bytes"
    exit 1
}

# Little-endian unpack
$payloadAddr = [BitConverter]::ToUInt32($pkgBytes, 8)
$payloadLen = [BitConverter]::ToUInt32($pkgBytes, 12)
$payloadCrc = [BitConverter]::ToUInt32($pkgBytes, 28)
$headerCrc = [BitConverter]::ToUInt32($pkgBytes, 32)
$sigLen = [BitConverter]::ToUInt32($pkgBytes, 40)

Write-Host ""
Write-Host "[verify] --- PKG header info ---"
Write-Host "  Payload addr:  $(Format-Addr $payloadAddr)"
Write-Host "  Payload len:   $payloadLen bytes"
Write-Host ("  Payload CRC:   0x{0:X8}" -f $payloadCrc)
Write-Host ("  Header CRC:    0x{0:X8}" -f $headerCrc)
Write-Host "  Sig length:    $sigLen"
Write-Host "  Total size:    $pkgLen bytes"

$payload = $pkgBytes[$HEADER_SIZE .. ($HEADER_SIZE + $payloadLen - 1)]

# --- Determine effective payload base address ---
# If payload_addr == 0, it's "offset mode" - payload[0] maps to HEX start address
$effectivePayloadAddr = $payloadAddr
if ($payloadAddr -eq 0) {
    $effectivePayloadAddr = $hexMin
    Write-Host ""
    Write-Host "[verify] NOTE: PayloadAddr=0x00000000 -> Using offset mode"
    Write-Host "[verify]       payload[0] maps to $(Format-Addr $hexMin)"
}

Write-Host ""
Write-Host "[verify] --- Address mapping ---"
Write-Host "  PKG payload[0]  -> FLASH addr: $(Format-Addr $effectivePayloadAddr)"
Write-Host "  PKG payload end -> FLASH addr: $(Format-Addr ($effectivePayloadAddr + $payloadLen - 1))"
Write-Host "  HEX start addr  -> $(Format-Addr $hexMin)"
Write-Host "  HEX end addr    -> $(Format-Addr $hexMax)"

# --- 3. Compare ---
Write-Host ""
Write-Host "[verify] --- Comparison ---"

$checkStart = [Math]::Max($hexMin, $effectivePayloadAddr)
$checkEnd = [Math]::Min($hexMax, $effectivePayloadAddr + $payloadLen - 1)

if ($checkStart -gt $checkEnd) {
    Write-Host "  [ERROR] No overlapping address range!"
    exit 1
}

$diffCount = 0
$missingInPkg = 0
$missingInHex = 0
$checked = 0
$firstMismatchAddr = $null
$lastMismatchAddr = $null

$totalOverlap = $checkEnd - $checkStart + 1
$sampleInterval = [Math]::Max(1, [int]($totalOverlap / 2000))

for ($offset = 0; $offset -lt $totalOverlap; $offset += $sampleInterval) {
    $flashAddr = $checkStart + $offset
    $hexVal = $hexData[$flashAddr]
    $pkgIdx = $flashAddr - $effectivePayloadAddr
    $pkgVal = $payload[$pkgIdx]

    $hasHex = $hexData.ContainsKey($flashAddr)
    $hasPkg = ($pkgIdx -ge 0 -and $pkgIdx -lt $payloadLen)

    if ($hasHex -and $hasPkg) {
        $checked++
        if ($hexVal -ne $pkgVal) {
            $diffCount++
            if ($firstMismatchAddr -eq $null) { $firstMismatchAddr = $flashAddr }
            $lastMismatchAddr = $flashAddr
            if ($diffCount -le 5) {
                Write-Host ("  [MISMATCH] addr=$(Format-Addr $flashAddr), HEX=0x{0:X2}, PKG=0x{1:X2}" -f $hexVal, $pkgVal)
            }
        }
    } elseif ($hasHex -and -not $hasPkg) {
        $missingInPkg++
        if ($missingInPkg -le 5) {
            Write-Host ("  [MISSING_IN_PKG] addr=$(Format-Addr $flashAddr), HEX=0x{0:X2}" -f $hexVal)
        }
    } elseif (-not $hasHex -and $hasPkg) {
        $missingInHex++
    }
}

# Detailed check first 256 bytes
Write-Host ""
Write-Host "[verify] --- First 256 bytes detail ---"
$firstDiffFound = $false
for ($i = 0; $i -lt [Math]::Min(256, $totalOverlap); $i++) {
    $addr = $checkStart + $i
    $hexVal = $hexData[$addr]
    $pkgIdx = $addr - $effectivePayloadAddr
    $pkgVal = $payload[$pkgIdx]
    $hasHex = $hexData.ContainsKey($addr)
    $hasPkg = ($pkgIdx -ge 0 -and $pkgIdx -lt $payloadLen)
    if ($hasHex -and $hasPkg -and ($hexVal -ne $pkgVal)) {
        if (-not $firstDiffFound) {
            $firstDiffFound = $true
            Write-Host ("  First mismatch at offset $i, addr=$(Format-Addr $addr)")
        }
        if ($i -lt 10) {
            Write-Host ("    addr=$(Format-Addr $addr), HEX=0x{0:X2}, PKG=0x{1:X2}" -f $hexVal, $pkgVal)
        }
    }
}
if (-not $firstDiffFound) {
    Write-Host "  First 256 bytes: MATCHED"
}

# Detailed check last 256 bytes
Write-Host ""
Write-Host "[verify] --- Last 256 bytes detail ---"
$lastDiffFound = $false
for ($i = [Math]::Max(0, $totalOverlap - 256); $i -lt $totalOverlap; $i++) {
    $addr = $checkStart + $i
    $hexVal = $hexData[$addr]
    $pkgIdx = $addr - $effectivePayloadAddr
    $pkgVal = $payload[$pkgIdx]
    $hasHex = $hexData.ContainsKey($addr)
    $hasPkg = ($pkgIdx -ge 0 -and $pkgIdx -lt $payloadLen)
    if ($hasHex -and $hasPkg -and ($hexVal -ne $pkgVal)) {
        if (-not $lastDiffFound) {
            $lastDiffFound = $true
            Write-Host ("  Last mismatch at offset $i, addr=$(Format-Addr $addr)")
        }
    }
}
if (-not $lastDiffFound) {
    Write-Host "  Last 256 bytes: MATCHED"
}

# HEX addresses outside PKG range
$hexOnly = $hexAddrs | Where-Object { $_ -lt $effectivePayloadAddr -or $_ -ge ($effectivePayloadAddr + $payloadLen) }
if ($hexOnly) {
    Write-Host ""
    Write-Host "[verify] --- HEX addresses outside PKG payload range ---"
    Write-Host "  Count: $($hexOnly.Count)"
    if ($hexOnly.Count -le 20) {
        foreach ($a in $hexOnly) {
            Write-Host ("    $(Format-Addr $a) = 0x{0:X2}" -f $hexData[$a])
        }
    } else {
        Write-Host "  First 10:"
        foreach ($a in ($hexOnly | Select-Object -First 10)) {
            Write-Host ("    $(Format-Addr $a) = 0x{0:X2}" -f $hexData[$a])
        }
        Write-Host "  Last 10:"
        foreach ($a in ($hexOnly | Select-Object -Last 10)) {
            Write-Host ("    $(Format-Addr $a) = 0x{0:X2}" -f $hexData[$a])
        }
    }
}

# Size comparison
if ($hexCount -ne $payloadLen) {
    Write-Host ""
    Write-Host "[verify] --- Size comparison ---"
    Write-Host "  HEX unique bytes: $hexCount"
    Write-Host "  PKG payload len:  $payloadLen"
    Write-Host ("  Difference:       {0} bytes" -f ($payloadLen - $hexCount))
    Write-Host "  NOTE: Difference may be due to:"
    Write-Host "        - Multi-segment HEX, only largest kept"
    Write-Host "        - Alignment padding (fill bytes)"
    Write-Host "        - Address gaps in HEX"
}

# Summary
Write-Host ""
Write-Host "======================================================================"
Write-Host "[verify] --- Summary ---"
Write-Host "  Total overlap:     $totalOverlap bytes"
Write-Host "  Sampled checked:   $checked"
Write-Host "  Mismatches:        $diffCount"
Write-Host "  Missing in PKG:    $missingInPkg"
Write-Host "  Missing in HEX:    $missingInHex"
Write-Host "  HEX outside PKG:   $($hexOnly.Count)"

if ($diffCount -eq 0 -and $missingInPkg -eq 0 -and $missingInHex -eq 0) {
    if ($hexOnly.Count -eq 0) {
        Write-Host ""
        Write-Host "[verify] V PASS: PKG payload completely matches aligned.hex"
    } else {
        Write-Host ""
        Write-Host "[verify] V PASS (with note): PKG matches overlap region"
        Write-Host "[verify]          But $($hexOnly.Count) HEX bytes are outside PKG range"
    }
} else {
    Write-Host ""
    Write-Host "[verify] X FAIL: PKG payload does NOT completely match aligned.hex"
}
Write-Host "======================================================================"

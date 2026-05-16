@echo off
chcp 65001 >nul
setlocal EnableDelayedExpansion

set PUB_KEY=%1
set OUT_H=%2

if "%PUB_KEY%"=="" (
    echo Usage: extract_pub.bat public.pem public_key.h
    exit /b 1
)
if "%OUT_H%"=="" (
    echo Usage: extract_pub.bat public.pem public_key.h
    exit /b 1
)

REM Extract modulus hex (remove "Modulus=" prefix)
for /f "tokens=2 delims==" %%a in ('openssl rsa -pubin -in "%PUB_KEY%" -modulus -noout 2^>nul') do set MOD_HEX=%%a

REM Convert to lowercase
set MOD_HEX=!MOD_HEX:A=a!
set MOD_HEX=!MOD_HEX:B=b!
set MOD_HEX=!MOD_HEX:C=c!
set MOD_HEX=!MOD_HEX:D=d!
set MOD_HEX=!MOD_HEX:E=e!
set MOD_HEX=!MOD_HEX:F=f!

echo /* Auto-generated public key header for RSA-2048 verification */ > "%OUT_H%"
echo /* Generated from %PUB_KEY% */ >> "%OUT_H%"
echo #ifndef CRYPTO_PUBLIC_KEY_H_ >> "%OUT_H%"
echo #define CRYPTO_PUBLIC_KEY_H_ >> "%OUT_H%"
echo. >> "%OUT_H%"
echo #define RSA_MODULUS_LEN   ^(256u^) >> "%OUT_H%"
echo #define RSA_EXPONENT_LEN  ^(4u^) >> "%OUT_H%"
echo. >> "%OUT_H%"
echo static const uint8 rsa_public_modulus[RSA_MODULUS_LEN] = >> "%OUT_H%"
echo { >> "%OUT_H%"

set /a idx=0
set "line=    "
:loop
if !idx! geq 512 goto done

set /a pos=idx
set byte=0x!MOD_HEX:~%pos%,2!

if !idx! equ 0 (
    set "line=    %byte%"
) else (
    if !idx! equ 504 (
        set "line=%line%, %byte%"
    ) else (
        set /a rem=idx %% 16
        if !rem! equ 0 (
            echo %line%, >> "%OUT_H%"
            set "line=    %byte%"
        ) else (
            set "line=%line%, %byte%"
        )
    )
)

set /a idx+=2
goto loop

:done
echo %line% >> "%OUT_H%"
echo }; >> "%OUT_H%"
echo. >> "%OUT_H%"
echo static const uint8 rsa_public_exponent[RSA_EXPONENT_LEN] = >> "%OUT_H%"
echo { >> "%OUT_H%"
echo     0x00, 0x01, 0x00, 0x01 >> "%OUT_H%"
echo }; >> "%OUT_H%"
echo. >> "%OUT_H%"
echo #endif /* CRYPTO_PUBLIC_KEY_H_ */ >> "%OUT_H%"

echo Generated: %OUT_H%

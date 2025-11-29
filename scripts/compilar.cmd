@echo off
REM Script de compilação para Windows (CMD)
REM Árvore-B de Ordem 3 - Banco de Dados de Imagens

echo Compilando arvore_b.c...

gcc -Wall -Wextra -std=c11 -O2 -o arvore_b.exe arvore_b.c

if %ERRORLEVEL% EQU 0 (
    echo [OK] Compilado com sucesso!
    echo Execute: arvore_b.exe
) else (
    echo [ERRO] Falha na compilacao
    exit /b 1
)

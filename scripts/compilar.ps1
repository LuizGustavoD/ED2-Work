# Script de compilação para Windows (PowerShell)
# Árvore-B de Ordem 3 - Banco de Dados de Imagens

Write-Host "Compilando arvore_b.c..." -ForegroundColor Cyan

gcc -Wall -Wextra -std=c11 -O2 -o arvore_b.exe arvore_b.c

if ($?) {
    Write-Host "[OK] Compilado com sucesso!" -ForegroundColor Green
    Write-Host "Execute: .\arvore_b.exe" -ForegroundColor Yellow
} else {
    Write-Host "[ERRO] Falha na compilacao" -ForegroundColor Red
    exit 1
}

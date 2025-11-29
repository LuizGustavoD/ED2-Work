#!/bin/bash
# Script de compilação para Linux/Mac
# Árvore-B de Ordem 3 - Banco de Dados de Imagens

echo -e "\033[0;36mCompilando arvore_b.c...\033[0m"

gcc -Wall -Wextra -std=c11 -O2 -o arvore_b arvore_b.c

if [ $? -eq 0 ]; then
    echo -e "\033[0;32m[OK] Compilado com sucesso!\033[0m"
    echo -e "\033[0;33mExecute: ./arvore_b\033[0m"
else
    echo -e "\033[0;31m[ERRO] Falha na compilacao\033[0m"
    exit 1
fi

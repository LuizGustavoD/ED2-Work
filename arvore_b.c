/*
 * ============================================================================
 * Árvore-B Paginada de Ordem 3 para Indexação de Banco de Dados de Imagens
 * Estrutura de Dados II - Trabalho 2
 * ============================================================================
 */

//Comando para compilação: gcc -Wall -Wextra -std=c11 -O2 -o arvore_b.exe arvore_b.c; if ($?) { Write-Host "[OK] Compilado com sucesso!" -ForegroundColor Green } else { Write-Host "[ERRO] Falha na compilacao" -ForegroundColor Red }

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


//Definições de constantes
#define ORDEM 3                          // Ordem da Árvore-B
#define MAX_CHAVES (ORDEM - 1)           // 2 chaves por nó
#define MIN_CHAVES ((ORDEM - 1) / 2)     // 1 chave mínima (exceto raiz)
#define MAX_FILHOS ORDEM                 // 3 filhos por nó

#define TAM_NOME_ARQUIVO 256
#define TAM_MAX_IMAGEM (640 * 480)       // 640x480 pixels

#define ARQUIVO_INDICE "models/indice.bin"
#define ARQUIVO_DADOS "models/dados.bin"

/**
 * Chave: Combina nome do arquivo e limiar aplicado
 * Usada para indexação na Árvore-B
 */
typedef struct {
    char nome_arquivo[TAM_NOME_ARQUIVO];
    int limiar;
    long offset_dados;                   
} Chave;

//Pagina: Estrutura de nó da Árvore-B
typedef struct {
    int num_chaves;                      
    Chave chaves[MAX_CHAVES];            
    long filhos[MAX_FILHOS];             
    bool eh_folha;                       
    long offset_proprio;                 
} Pagina;

/**
 * Cabeçalho do arquivo de índice
 * Mantém metadados da Árvore-B
 */
typedef struct {
    long offset_raiz;                    // Offset da raiz
    long proximo_offset;                 // Próximo espaço livre
    int altura;                          // Altura da árvore
    int num_paginas;                     // Total de páginas
} CabecalhoIndice;

/**
 * Registro de imagem no arquivo de dados
 */
typedef struct {
    char nome_original[TAM_NOME_ARQUIVO];
    int limiar;
    int largura;
    int altura;
    int max_valor;
    unsigned char dados[TAM_MAX_IMAGEM];
} RegistroImagem;

/**
 * Estrutura principal do banco de dados
 */
typedef struct {
    FILE *arquivo_indice;
    FILE *arquivo_dados;
    Pagina *raiz_ram;                    
    CabecalhoIndice cabecalho;
} BancoDados;

// Declarações de funções
void remover_recursivo(BancoDados *bd, Pagina *pagina, Chave *chave);
void preencher(BancoDados *bd, Pagina *pagina, int idx);
long compactar_paginas_recursivo(BancoDados *bd, Pagina *pagina, FILE *temp_indice, CabecalhoIndice *novo_cabecalho);


// Funções auxiliares
int comparar_chaves(const Chave *a, const Chave *b) {
    int cmp = strcmp(a->nome_arquivo, b->nome_arquivo);
    if (cmp != 0) return cmp;
    return a->limiar - b->limiar;
}

Pagina* criar_pagina(bool eh_folha) {
    Pagina *pagina = (Pagina*)calloc(1, sizeof(Pagina));
    pagina->num_chaves = 0;
    pagina->eh_folha = eh_folha;
    pagina->offset_proprio = -1;
    
    for (int i = 0; i < MAX_FILHOS; i++) {
        pagina->filhos[i] = -1;
    }
    
    return pagina;
}

// Funções de leitura e escrita de arquivos
void escrever_cabecalho(FILE *arquivo, CabecalhoIndice *cab) {
    fseek(arquivo, 0, SEEK_SET);
    fwrite(cab, sizeof(CabecalhoIndice), 1, arquivo);
    fflush(arquivo);
}

/**
 * Lê o cabeçalho do arquivo de índice
 */
void ler_cabecalho(FILE *arquivo, CabecalhoIndice *cab) {
    fseek(arquivo, 0, SEEK_SET);
    fread(cab, sizeof(CabecalhoIndice), 1, arquivo);
}

void escrever_pagina(FILE *arquivo, Pagina *pagina, long offset) {
    fseek(arquivo, offset, SEEK_SET);
    fwrite(pagina, sizeof(Pagina), 1, arquivo);
    fflush(arquivo);
}

Pagina* ler_pagina(FILE *arquivo, long offset) {
    if (offset == -1) return NULL;
    
    Pagina *pagina = (Pagina*)malloc(sizeof(Pagina));
    fseek(arquivo, offset, SEEK_SET);
    fread(pagina, sizeof(Pagina), 1, arquivo);
    
    return pagina;
}

long alocar_pagina(BancoDados *bd) {
    long offset = bd->cabecalho.proximo_offset;
    bd->cabecalho.proximo_offset += sizeof(Pagina);
    bd->cabecalho.num_paginas++;
    escrever_cabecalho(bd->arquivo_indice, &bd->cabecalho);
    return offset;
}

// Funções de busca
int buscar_posicao(Pagina *pagina, Chave *chave) {
    int i = 0;
    while (i < pagina->num_chaves && comparar_chaves(chave, &pagina->chaves[i]) > 0) {
        i++;
    }
    return i;
}

/**
 * Busca uma chave na árvore
 * Retorna true se encontrada, false caso contrário
 */
bool buscar(BancoDados *bd, Chave *chave, Chave *resultado) {
    Pagina *pagina_atual = bd->raiz_ram;
    
    while (pagina_atual != NULL) {
        int i = buscar_posicao(pagina_atual, chave);
        
        if (i < pagina_atual->num_chaves && 
            comparar_chaves(chave, &pagina_atual->chaves[i]) == 0) {
            if (resultado) {
                *resultado = pagina_atual->chaves[i];
            }
            if (pagina_atual != bd->raiz_ram) {
                free(pagina_atual);
            }
            return true;
        }
        

        if (pagina_atual->eh_folha) {
            if (pagina_atual != bd->raiz_ram) {
                free(pagina_atual);
            }
            return false;
        }
        
        long offset_filho = pagina_atual->filhos[i];
        if (pagina_atual != bd->raiz_ram) {
            free(pagina_atual);
        }
        pagina_atual = ler_pagina(bd->arquivo_indice, offset_filho);
    }
    
    return false;
}

//Funções de inserção
void dividir_filho(BancoDados *bd, Pagina *pai, int indice, Pagina *filho_cheio) {
    Pagina *novo_filho = criar_pagina(filho_cheio->eh_folha);
    novo_filho->num_chaves = 0;  // Inicialmente vazio
    novo_filho->offset_proprio = alocar_pagina(bd);
    
    // Se não é folha, move o último filho para o novo nó
    if (!filho_cheio->eh_folha) {
        novo_filho->filhos[0] = filho_cheio->filhos[2];
    }
    
    filho_cheio->num_chaves = 1;  // Fica com K0
    
    // Move chaves e filhos do pai para abrir espaço
    for (int j = pai->num_chaves; j > indice; j--) {
        pai->chaves[j] = pai->chaves[j - 1];
    }
    for (int j = pai->num_chaves + 1; j > indice + 1; j--) {
        pai->filhos[j] = pai->filhos[j - 1];
    }
    
    // Insere a chave do meio no pai
    pai->chaves[indice] = filho_cheio->chaves[MIN_CHAVES];
    pai->filhos[indice + 1] = novo_filho->offset_proprio;
    pai->num_chaves++;
    
    // Escreve as páginas no disco
    escrever_pagina(bd->arquivo_indice, filho_cheio, filho_cheio->offset_proprio);
    escrever_pagina(bd->arquivo_indice, novo_filho, novo_filho->offset_proprio);
    
    free(novo_filho);
}

/**
 * Insere em uma página que não está cheia
 */
void inserir_nao_cheio(BancoDados *bd, Pagina *pagina, Chave *chave) {
    int i = pagina->num_chaves - 1;
    
    if (pagina->eh_folha) {
        // Insere diretamente na folha
        while (i >= 0 && comparar_chaves(chave, &pagina->chaves[i]) < 0) {
            pagina->chaves[i + 1] = pagina->chaves[i];
            i--;
        }
        pagina->chaves[i + 1] = *chave;
        pagina->num_chaves++;
        
        if (pagina == bd->raiz_ram) {
            escrever_pagina(bd->arquivo_indice, pagina, pagina->offset_proprio);
        }
    } else {
        // Encontra o filho onde deve inserir
        while (i >= 0 && comparar_chaves(chave, &pagina->chaves[i]) < 0) {
            i--;
        }
        i++;
        
        Pagina *filho = ler_pagina(bd->arquivo_indice, pagina->filhos[i]);
        
        if (filho->num_chaves == MAX_CHAVES) {
            // Filho está cheio, divide
            dividir_filho(bd, pagina, i, filho);
            
            if (comparar_chaves(chave, &pagina->chaves[i]) > 0) {
                i++;
            }
            
            free(filho);
            filho = ler_pagina(bd->arquivo_indice, pagina->filhos[i]);
        }
        
        inserir_nao_cheio(bd, filho, chave);
        escrever_pagina(bd->arquivo_indice, filho, filho->offset_proprio);
        free(filho);
    }
}

/**
 * Insere uma chave na árvore
 */
void inserir(BancoDados *bd, Chave *chave) {
    Pagina *raiz = bd->raiz_ram;
    
    if (raiz->num_chaves == MAX_CHAVES) {
        // Raiz está cheia, cria nova raiz
        Pagina *nova_raiz = criar_pagina(false);
        nova_raiz->offset_proprio = alocar_pagina(bd);
        nova_raiz->filhos[0] = raiz->offset_proprio;
        
        // Escreve a raiz antiga no disco
        escrever_pagina(bd->arquivo_indice, raiz, raiz->offset_proprio);
        
        dividir_filho(bd, nova_raiz, 0, raiz);
        
        // Atualiza a raiz
        bd->raiz_ram = nova_raiz;
        free(raiz);
        raiz = nova_raiz;
        
        bd->cabecalho.offset_raiz = nova_raiz->offset_proprio;
        bd->cabecalho.altura++;
        escrever_cabecalho(bd->arquivo_indice, &bd->cabecalho);
    }
    
    inserir_nao_cheio(bd, raiz, chave);
    escrever_pagina(bd->arquivo_indice, raiz, raiz->offset_proprio);
}

//Funções de remoção
Chave obter_predecessor(BancoDados *bd, Pagina *pagina, int idx) {
    Pagina *atual = ler_pagina(bd->arquivo_indice, pagina->filhos[idx]);
    while (!atual->eh_folha) {
        Pagina *proximo = ler_pagina(bd->arquivo_indice, atual->filhos[atual->num_chaves]);
        free(atual);
        atual = proximo;
    }
    Chave pred = atual->chaves[atual->num_chaves - 1];
    free(atual);
    return pred;
}

/**
 * Obtém a chave sucessora (menor chave da subárvore direita)
 */
Chave obter_sucessor(BancoDados *bd, Pagina *pagina, int idx) {
    Pagina *atual = ler_pagina(bd->arquivo_indice, pagina->filhos[idx + 1]);
    while (!atual->eh_folha) {
        Pagina *proximo = ler_pagina(bd->arquivo_indice, atual->filhos[0]);
        free(atual);
        atual = proximo;
    }
    Chave suc = atual->chaves[0];
    free(atual);
    return suc;
}

/**
 * Faz merge de um filho com seu irmão
 */
void merge(BancoDados *bd, Pagina *pagina, int idx) {
    Pagina *filho = ler_pagina(bd->arquivo_indice, pagina->filhos[idx]);
    Pagina *irmao = ler_pagina(bd->arquivo_indice, pagina->filhos[idx + 1]);
    
    // Puxa a chave do pai para o filho
    filho->chaves[filho->num_chaves] = pagina->chaves[idx];
    filho->num_chaves++;
    
    // Copia chaves do irmão
    for (int i = 0; i < irmao->num_chaves; i++) {
        filho->chaves[filho->num_chaves] = irmao->chaves[i];
        filho->num_chaves++;
    }
    
    // Copia filhos do irmão (se não for folha)
    if (!filho->eh_folha) {
        int pos = filho->num_chaves - irmao->num_chaves - 1;
        for (int i = 0; i <= irmao->num_chaves; i++) {
            filho->filhos[pos + i] = irmao->filhos[i];
        }
    }
    
    for (int i = idx; i < pagina->num_chaves - 1; i++) {
        pagina->chaves[i] = pagina->chaves[i + 1];
    }
    
    for (int i = idx + 1; i < pagina->num_chaves; i++) {
        pagina->filhos[i] = pagina->filhos[i + 1];
    }
    
    pagina->num_chaves--;
    
    escrever_pagina(bd->arquivo_indice, filho, filho->offset_proprio);
    escrever_pagina(bd->arquivo_indice, pagina, pagina->offset_proprio);
    
    free(filho);
    free(irmao);
}

void emprestar_do_anterior(BancoDados *bd, Pagina *pagina, int idx) {
    Pagina *filho = ler_pagina(bd->arquivo_indice, pagina->filhos[idx]);
    Pagina *irmao = ler_pagina(bd->arquivo_indice, pagina->filhos[idx - 1]);
    
    // Move chaves do filho para frente
    for (int i = filho->num_chaves - 1; i >= 0; i--) {
        filho->chaves[i + 1] = filho->chaves[i];
    }
    
    // Move filhos (se não for folha)
    if (!filho->eh_folha) {
        for (int i = filho->num_chaves; i >= 0; i--) {
            filho->filhos[i + 1] = filho->filhos[i];
        }
    }
    
    filho->chaves[0] = pagina->chaves[idx - 1];
    
    pagina->chaves[idx - 1] = irmao->chaves[irmao->num_chaves - 1];
    
    if (!filho->eh_folha) {
        filho->filhos[0] = irmao->filhos[irmao->num_chaves];
    }
    
    filho->num_chaves++;
    irmao->num_chaves--;
    
    escrever_pagina(bd->arquivo_indice, filho, filho->offset_proprio);
    escrever_pagina(bd->arquivo_indice, irmao, irmao->offset_proprio);
    escrever_pagina(bd->arquivo_indice, pagina, pagina->offset_proprio);
    
    free(filho);
    free(irmao);
}

/**
 * Empresta uma chave do irmão seguinte
 */
void emprestar_do_proximo(BancoDados *bd, Pagina *pagina, int idx) {
    Pagina *filho = ler_pagina(bd->arquivo_indice, pagina->filhos[idx]);
    Pagina *irmao = ler_pagina(bd->arquivo_indice, pagina->filhos[idx + 1]);
    
    // Move chave do pai para o filho
    filho->chaves[filho->num_chaves] = pagina->chaves[idx];
    
    // Move primeiro filho do irmão
    if (!filho->eh_folha) {
        filho->filhos[filho->num_chaves + 1] = irmao->filhos[0];
    }
    
    // Move primeira chave do irmão para o pai
    pagina->chaves[idx] = irmao->chaves[0];
    
    // Move chaves do irmão
    for (int i = 1; i < irmao->num_chaves; i++) {
        irmao->chaves[i - 1] = irmao->chaves[i];
    }
    
    // Move filhos do irmão
    if (!irmao->eh_folha) {
        for (int i = 1; i <= irmao->num_chaves; i++) {
            irmao->filhos[i - 1] = irmao->filhos[i];
        }
    }
    
    filho->num_chaves++;
    irmao->num_chaves--;
    
    escrever_pagina(bd->arquivo_indice, filho, filho->offset_proprio);
    escrever_pagina(bd->arquivo_indice, irmao, irmao->offset_proprio);
    escrever_pagina(bd->arquivo_indice, pagina, pagina->offset_proprio);
    
    free(filho);
    free(irmao);
}

/**
 * Preenche um filho que tem poucas chaves
 */
void preencher(BancoDados *bd, Pagina *pagina, int idx) {
    // Tenta emprestar do irmão anterior
    if (idx != 0) {
        Pagina *irmao_ant = ler_pagina(bd->arquivo_indice, pagina->filhos[idx - 1]);
        // Só empresta se irmão tem mais que MIN_CHAVES
        if (irmao_ant->num_chaves > MIN_CHAVES) {
            free(irmao_ant);
            emprestar_do_anterior(bd, pagina, idx);
            return;
        }
        free(irmao_ant);
    }
    
    // Tenta emprestar do irmão seguinte
    if (idx != pagina->num_chaves) {
        Pagina *irmao_prox = ler_pagina(bd->arquivo_indice, pagina->filhos[idx + 1]);
        // Só empresta se irmão tem mais que MIN_CHAVES
        if (irmao_prox->num_chaves > MIN_CHAVES) {
            free(irmao_prox);
            emprestar_do_proximo(bd, pagina, idx);
            return;
        }
        free(irmao_prox);
    }
    
    // Não pode emprestar: faz merge
    // Dá preferência para merge com irmão direito se ele estiver vazio
    if (idx != pagina->num_chaves) {
        Pagina *irmao_dir = ler_pagina(bd->arquivo_indice, pagina->filhos[idx + 1]);
        if (irmao_dir->num_chaves == 0) {
            free(irmao_dir);
            merge(bd, pagina, idx);  // Merge com irmão direito vazio
            return;
        }
        free(irmao_dir);
        merge(bd, pagina, idx);
    } else {
        merge(bd, pagina, idx - 1);
    }
}

/**
 * Remove uma chave de uma folha
 */
void remover_de_folha(Pagina *pagina, int idx) {
    for (int i = idx + 1; i < pagina->num_chaves; i++) {
        pagina->chaves[i - 1] = pagina->chaves[i];
    }
    pagina->num_chaves--;
}

/**
 * Remove uma chave de um nó interno
 */
void remover_de_nao_folha(BancoDados *bd, Pagina *pagina, int idx) {
    Chave chave = pagina->chaves[idx];
    
    Pagina *filho = ler_pagina(bd->arquivo_indice, pagina->filhos[idx]);
    Pagina *irmao = ler_pagina(bd->arquivo_indice, pagina->filhos[idx + 1]);
    
    if (filho->num_chaves > MIN_CHAVES) {
        // Substitui pela chave predecessora
        Chave pred = obter_predecessor(bd, pagina, idx);
        pagina->chaves[idx] = pred;
        free(filho);
        free(irmao);
        
        filho = ler_pagina(bd->arquivo_indice, pagina->filhos[idx]);
        remover_recursivo(bd, filho, &pred);
        escrever_pagina(bd->arquivo_indice, filho, filho->offset_proprio);
        free(filho);
    } else if (irmao->num_chaves > MIN_CHAVES) {
        // Substitui pela chave sucessora
        Chave suc = obter_sucessor(bd, pagina, idx);
        pagina->chaves[idx] = suc;
        free(filho);
        free(irmao);
        
        irmao = ler_pagina(bd->arquivo_indice, pagina->filhos[idx + 1]);
        remover_recursivo(bd, irmao, &suc);
        escrever_pagina(bd->arquivo_indice, irmao, irmao->offset_proprio);
        free(irmao);
    } else {
        // Ambos filhos têm MIN_CHAVES: faz merge e remove do resultado
        free(filho);
        free(irmao);
        merge(bd, pagina, idx);
        // Após merge, a chave já foi movida para o filho fundido
        // Agora remove do filho fundido
        filho = ler_pagina(bd->arquivo_indice, pagina->filhos[idx]);
        remover_recursivo(bd, filho, &chave);
        escrever_pagina(bd->arquivo_indice, filho, filho->offset_proprio);
        free(filho);
    }
}

/**
 * Remove uma chave recursivamente
 */
void remover_recursivo(BancoDados *bd, Pagina *pagina, Chave *chave) {
    int idx = buscar_posicao(pagina, chave);
    
    if (idx < pagina->num_chaves && comparar_chaves(chave, &pagina->chaves[idx]) == 0) {
        if (pagina->eh_folha) {
            remover_de_folha(pagina, idx);
        } else {
            remover_de_nao_folha(bd, pagina, idx);
        }
    } else {
        if (pagina->eh_folha) {
            return; // Chave não encontrada
        }
        
        bool flag = (idx == pagina->num_chaves);
        
        // Preenche preventivamente se o filho está no mínimo
        Pagina *filho = ler_pagina(bd->arquivo_indice, pagina->filhos[idx]);
        if (filho->num_chaves <= MIN_CHAVES) {
            free(filho);
            preencher(bd, pagina, idx);
            
            // Após preencher, recarrega o filho correto
            if (flag && idx > pagina->num_chaves) {
                filho = ler_pagina(bd->arquivo_indice, pagina->filhos[idx - 1]);
            } else {
                filho = ler_pagina(bd->arquivo_indice, pagina->filhos[idx]);
            }
        }
        
        remover_recursivo(bd, filho, chave);
        escrever_pagina(bd->arquivo_indice, filho, filho->offset_proprio);
        free(filho);
    }
}

/**
 * Remove uma chave da árvore
 */
bool remover(BancoDados *bd, Chave *chave) {
    if (!buscar(bd, chave, NULL)) {
        return false;
    }
    
    remover_recursivo(bd, bd->raiz_ram, chave);
    
    // Se a raiz ficou vazia
    if (bd->raiz_ram->num_chaves == 0) {
        if (!bd->raiz_ram->eh_folha) {
            // Raiz interna vazia: promove o único filho restante
            long novo_offset_raiz = bd->raiz_ram->filhos[0];
            Pagina *nova_raiz = ler_pagina(bd->arquivo_indice, novo_offset_raiz);
            
            // Marca a antiga raiz como inválida
            long offset_raiz_antiga = bd->raiz_ram->offset_proprio;
            bd->raiz_ram->num_chaves = -1;
            escrever_pagina(bd->arquivo_indice, bd->raiz_ram, offset_raiz_antiga);
            
            free(bd->raiz_ram);
            bd->raiz_ram = nova_raiz;
            
            bd->cabecalho.offset_raiz = novo_offset_raiz;
            bd->cabecalho.altura--;
            escrever_cabecalho(bd->arquivo_indice, &bd->cabecalho);
            
            // Escreve nova raiz
            escrever_pagina(bd->arquivo_indice, bd->raiz_ram, bd->raiz_ram->offset_proprio);
            return true;
        }
    }
    
    // Limpa filhos vazios após remoção (múltiplas passadas se necessário)
    bool houve_merge = true;
    while (houve_merge && !bd->raiz_ram->eh_folha && bd->raiz_ram->num_chaves > 0) {
        houve_merge = false;
        for (int i = 0; i <= bd->raiz_ram->num_chaves; i++) {
            Pagina *filho = ler_pagina(bd->arquivo_indice, bd->raiz_ram->filhos[i]);
            if (filho->num_chaves == 0) {
                free(filho);
                // Funde filho vazio com irmão
                if (i > 0) {
                    merge(bd, bd->raiz_ram, i - 1);
                    houve_merge = true;
                    break;
                } else if (i < bd->raiz_ram->num_chaves) {
                    merge(bd, bd->raiz_ram, i);
                    houve_merge = true;
                    break;
                }
            } else {
                free(filho);
            }
        }
    }
    
    escrever_pagina(bd->arquivo_indice, bd->raiz_ram, bd->raiz_ram->offset_proprio);
    
    // Verifica novamente se raiz ficou vazia após merges
    if (bd->raiz_ram->num_chaves == 0 && !bd->raiz_ram->eh_folha) {
        long novo_offset_raiz = bd->raiz_ram->filhos[0];
        Pagina *nova_raiz = ler_pagina(bd->arquivo_indice, novo_offset_raiz);
        
        long offset_raiz_antiga = bd->raiz_ram->offset_proprio;
        bd->raiz_ram->num_chaves = -1;
        escrever_pagina(bd->arquivo_indice, bd->raiz_ram, offset_raiz_antiga);
        
        free(bd->raiz_ram);
        bd->raiz_ram = nova_raiz;
        
        bd->cabecalho.offset_raiz = novo_offset_raiz;
        bd->cabecalho.altura--;
        escrever_cabecalho(bd->arquivo_indice, &bd->cabecalho);
        escrever_pagina(bd->arquivo_indice, bd->raiz_ram, bd->raiz_ram->offset_proprio);
    }
    
    return true;
}

// Funções de percurso
void percurso_em_ordem_recursivo(BancoDados *bd, Pagina *pagina) {
    if (pagina == NULL) return;
    
    int i;
    for (i = 0; i < pagina->num_chaves; i++) {
        if (!pagina->eh_folha) {
            Pagina *filho = ler_pagina(bd->arquivo_indice, pagina->filhos[i]);
            percurso_em_ordem_recursivo(bd, filho);
            free(filho);
        }
        printf("  %s, limiar=%d (offset: %ld)\n", 
               pagina->chaves[i].nome_arquivo, 
               pagina->chaves[i].limiar,
               pagina->chaves[i].offset_dados);
    }
    
    if (!pagina->eh_folha) {
        Pagina *filho = ler_pagina(bd->arquivo_indice, pagina->filhos[i]);
        percurso_em_ordem_recursivo(bd, filho);
        free(filho);
    }
}

/**
 * Percurso em ordem - interface pública
 */
void percurso_em_ordem(BancoDados *bd) {
    printf("\n=== Percurso em Ordem (Chaves Ordenadas) ===\n");
    percurso_em_ordem_recursivo(bd, bd->raiz_ram);
    printf("============================================\n\n");
}

// Funções de visualização de páginas
void imprimir_pagina(Pagina *pagina, int num_pagina) {
    printf("Página [%d]: (offset: %ld, folha: %s, chaves: %d)\n", 
           num_pagina, 
           pagina->offset_proprio,
           pagina->eh_folha ? "SIM" : "NÃO",
           pagina->num_chaves);
    
    for (int i = 0; i < pagina->num_chaves; i++) {
        printf("  Chave [%s], limiar=[%d], offset_dados=%ld\n",
               pagina->chaves[i].nome_arquivo,
               pagina->chaves[i].limiar,
               pagina->chaves[i].offset_dados);
    }
    
    if (!pagina->eh_folha) {
        printf("  Filhos: ");
        for (int i = 0; i <= pagina->num_chaves; i++) {
            printf("[%ld] ", pagina->filhos[i]);
        }
        printf("\n");
    }
    printf("\n");
}

/**
 * Visualiza todas as páginas do arquivo de índice
 */
void visualizar_paginas(BancoDados *bd) {
    printf("\n=== Visualização de Páginas do Índice ===\n");
    printf("Total de páginas: %d\n", bd->cabecalho.num_paginas);
    printf("Altura da árvore: %d\n\n", bd->cabecalho.altura);
    
    long offset = sizeof(CabecalhoIndice);
    int num_pagina = 0;
    
    while (offset < bd->cabecalho.proximo_offset) {
        Pagina *pagina = ler_pagina(bd->arquivo_indice, offset);
        imprimir_pagina(pagina, num_pagina);
        free(pagina);
        
        offset += sizeof(Pagina);
        num_pagina++;
    }
    
    printf("=========================================\n\n");
}

// Funções de manipulação de imagens
void aplicar_limiarizacao(RegistroImagem *img_orig, RegistroImagem *img_bin, int limiar) {
    *img_bin = *img_orig;
    img_bin->limiar = limiar;
    
    int total_pixels = img_orig->largura * img_orig->altura;
    for (int i = 0; i < total_pixels; i++) {
        img_bin->dados[i] = (img_orig->dados[i] >= limiar) ? 255 : 0;
    }
}

/**
 * Lê arquivo PGM
 */
bool ler_pgm(const char *nome_arquivo, RegistroImagem *img) {
    FILE *fp = fopen(nome_arquivo, "rb");
    if (!fp) {
        printf("Erro ao abrir arquivo %s\n", nome_arquivo);
        return false;
    }
    
    char formato[3];
    fscanf(fp, "%2s", formato);
    
    if (strcmp(formato, "P5") != 0 && strcmp(formato, "P2") != 0) {
        printf("Formato não suportado (apenas P2 e P5)\n");
        fclose(fp);
        return false;
    }
    
    // Ignora comentários
    char c = getc(fp);
    while (c == '#') {
        while (getc(fp) != '\n');
        c = getc(fp);
    }
    ungetc(c, fp);
    
    fscanf(fp, "%d %d", &img->largura, &img->altura);
    fscanf(fp, "%d", &img->max_valor);
    fgetc(fp); // Consome newline
    
    int total_pixels = img->largura * img->altura;
    
    if (total_pixels > TAM_MAX_IMAGEM) {
        printf("Imagem muito grande (máximo %d pixels)\n", TAM_MAX_IMAGEM);
        fclose(fp);
        return false;
    }
    
    if (strcmp(formato, "P5") == 0) {
        fread(img->dados, sizeof(unsigned char), total_pixels, fp);
    } else {
        for (int i = 0; i < total_pixels; i++) {
            int valor;
            fscanf(fp, "%d", &valor);
            img->dados[i] = (unsigned char)valor;
        }
    }
    
    fclose(fp);
    strcpy(img->nome_original, nome_arquivo);
    return true;
}

/**
 * Salva imagem no arquivo de dados
 */
long salvar_imagem(FILE *arquivo_dados, RegistroImagem *img) {
    fseek(arquivo_dados, 0, SEEK_END);
    long offset = ftell(arquivo_dados);
    fwrite(img, sizeof(RegistroImagem), 1, arquivo_dados);
    fflush(arquivo_dados);
    return offset;
}

/**
 * Carrega imagem do arquivo de dados
 */
bool carregar_imagem(FILE *arquivo_dados, long offset, RegistroImagem *img) {
    fseek(arquivo_dados, offset, SEEK_SET);
    size_t lido = fread(img, sizeof(RegistroImagem), 1, arquivo_dados);
    return lido == 1;
}

/**
 * Exporta imagem para arquivo PGM
 */
bool exportar_pgm(RegistroImagem *img, const char *nome_saida, bool formato_p2) {
    FILE *fp = fopen(nome_saida, "wb");
    if (!fp) {
        printf("Erro ao criar arquivo %s\n", nome_saida);
        return false;
    }
    
    if (formato_p2) {
        // Formato P2 (ASCII)
        fprintf(fp, "P2\n");
        fprintf(fp, "%d %d\n", img->largura, img->altura);
        fprintf(fp, "%d\n", img->max_valor);
        
        int total_pixels = img->largura * img->altura;
        for (int i = 0; i < total_pixels; i++) {
            fprintf(fp, "%d ", img->dados[i]);
            if ((i + 1) % 20 == 0) fprintf(fp, "\n"); // 20 valores por linha
        }
        fprintf(fp, "\n");
    } else {
        // Formato P5 (binário)
        fprintf(fp, "P5\n");
        fprintf(fp, "%d %d\n", img->largura, img->altura);
        fprintf(fp, "%d\n", img->max_valor);
        
        int total_pixels = img->largura * img->altura;
        fwrite(img->dados, sizeof(unsigned char), total_pixels, fp);
    }
    
    fclose(fp);
    return true;
}

// Funções de compactação
typedef struct {
    Chave *chaves;
    int num_chaves;
    int capacidade;
} ListaChaves;

/**
 * Coleta todas as chaves em ordem (para compactação)
 */
void coletar_chaves_recursivo(BancoDados *bd, Pagina *pagina, ListaChaves *lista) {
    if (pagina == NULL) return;
    
    int i;
    for (i = 0; i < pagina->num_chaves; i++) {
        if (!pagina->eh_folha) {
            Pagina *filho = ler_pagina(bd->arquivo_indice, pagina->filhos[i]);
            coletar_chaves_recursivo(bd, filho, lista);
            free(filho);
        }
        
        if (lista->num_chaves >= lista->capacidade) {
            lista->capacidade *= 2;
            lista->chaves = realloc(lista->chaves, lista->capacidade * sizeof(Chave));
        }
        lista->chaves[lista->num_chaves++] = pagina->chaves[i];
    }
    
    if (!pagina->eh_folha) {
        Pagina *filho = ler_pagina(bd->arquivo_indice, pagina->filhos[i]);
        coletar_chaves_recursivo(bd, filho, lista);
        free(filho);
    }
}

/**
 * Busca o novo offset de uma chave na lista compactada
 */
long buscar_novo_offset(Chave *chave_antiga, ListaChaves *lista) {
    for (int i = 0; i < lista->num_chaves; i++) {
        if (strcmp(lista->chaves[i].nome_arquivo, chave_antiga->nome_arquivo) == 0 &&
            lista->chaves[i].limiar == chave_antiga->limiar) {
            return lista->chaves[i].offset_dados;
        }
    }
    return -1; // Não encontrado
}

/**
 * Atualiza offsets recursivamente em todas as páginas da árvore
 */
void atualizar_offsets_recursivo(BancoDados *bd, Pagina *pagina, ListaChaves *lista) {
    bool modificado = false;
    
    // Atualiza offsets das chaves nesta página
    for (int i = 0; i < pagina->num_chaves; i++) {
        long novo_offset = buscar_novo_offset(&pagina->chaves[i], lista);
        if (novo_offset != -1 && novo_offset != pagina->chaves[i].offset_dados) {
            pagina->chaves[i].offset_dados = novo_offset;
            modificado = true;
        }
    }
    
    // Se a página foi modificada, grava de volta
    if (modificado) {
        escrever_pagina(bd->arquivo_indice, pagina, pagina->offset_proprio);
    }
    
    // Processa filhos recursivamente (se não for folha)
    if (!pagina->eh_folha) {
        for (int i = 0; i <= pagina->num_chaves; i++) {
            Pagina *filho = ler_pagina(bd->arquivo_indice, pagina->filhos[i]);
            atualizar_offsets_recursivo(bd, filho, lista);
            free(filho);
        }
    }
}

/**
 * Compacta páginas recursivamente, escrevendo apenas páginas válidas
 * Retorna o novo offset da página escrita
 */
long compactar_paginas_recursivo(BancoDados *bd, Pagina *pagina, FILE *temp_indice, CabecalhoIndice *novo_cabecalho) {
    // Ignora páginas inválidas (marcadas com num_chaves < 0)
    if (pagina->num_chaves < 0) {
        return -1;
    }
    
    // Se não é folha, compacta filhos primeiro e atualiza ponteiros
    if (!pagina->eh_folha) {
        long novos_filhos[MAX_FILHOS];
        int filhos_validos = 0;
        
        for (int i = 0; i <= pagina->num_chaves; i++) {
            Pagina *filho = ler_pagina(bd->arquivo_indice, pagina->filhos[i]);
            long novo_offset_filho = compactar_paginas_recursivo(bd, filho, temp_indice, novo_cabecalho);
            free(filho);
            
            if (novo_offset_filho != -1) {
                novos_filhos[filhos_validos++] = novo_offset_filho;
            }
        }
        
        // Atualiza ponteiros para filhos
        for (int i = 0; i < filhos_validos; i++) {
            pagina->filhos[i] = novos_filhos[i];
        }
    }
    
    // Escreve esta página no arquivo compactado
    long novo_offset = ftell(temp_indice);
    pagina->offset_proprio = novo_offset;
    fwrite(pagina, sizeof(Pagina), 1, temp_indice);
    
    novo_cabecalho->num_paginas++;
    novo_cabecalho->proximo_offset = ftell(temp_indice);
    
    return novo_offset;
}

/**
 * Compacta o arquivo de dados
 * Utiliza percurso em ordem para reorganizar os dados
 * Mantém a estrutura do índice intacta, apenas atualizando offsets
 */
void compactar(BancoDados *bd) {
    printf("Iniciando compactacao do arquivo de dados...\n");
    
    // Coleta todas as chaves em ordem
    ListaChaves lista;
    lista.capacidade = 100;
    lista.num_chaves = 0;
    lista.chaves = malloc(lista.capacidade * sizeof(Chave));
    
    coletar_chaves_recursivo(bd, bd->raiz_ram, &lista);
    
    if (lista.num_chaves == 0) {
        printf("Nenhuma imagem para compactar.\n");
        free(lista.chaves);
        return;
    }
    
    printf("Coletadas %d chaves. Reorganizando arquivo de dados...\n", lista.num_chaves);
    
    // === COMPACTAÇÃO DO ARQUIVO DE DADOS ===
    FILE *temp_dados = fopen("models/dados_temp.bin", "wb");
    if (!temp_dados) {
        printf("Erro ao criar arquivo temporario de dados.\n");
        free(lista.chaves);
        return;
    }
    
    // Copia imagens para o arquivo temporário e atualiza offsets na lista
    for (int i = 0; i < lista.num_chaves; i++) {
        RegistroImagem img;
        if (carregar_imagem(bd->arquivo_dados, lista.chaves[i].offset_dados, &img)) {
            long novo_offset = ftell(temp_dados);
            fwrite(&img, sizeof(RegistroImagem), 1, temp_dados);
            lista.chaves[i].offset_dados = novo_offset;
        }
    }
    
    fclose(temp_dados);
    fclose(bd->arquivo_dados);
    
    // Substitui o arquivo original de dados
    remove(ARQUIVO_DADOS);
    rename("models/dados_temp.bin", ARQUIVO_DADOS);
    
    // Reabre o arquivo de dados
    bd->arquivo_dados = fopen(ARQUIVO_DADOS, "r+b");
    
    printf("Atualizando offsets no indice...\n");
    
    // Atualiza offsets em todas as páginas da árvore (mantendo estrutura)
    atualizar_offsets_recursivo(bd, bd->raiz_ram, &lista);
    
    // Compacta o arquivo de índice
    printf("Compactando arquivo de indice...\n");
    
    // Cria arquivo temporário para índice
    FILE *temp_indice = fopen("models/indice_temp.bin", "wb");
    if (!temp_indice) {
        printf("Erro ao criar arquivo temporario de indice.\n");
        free(lista.chaves);
        return;
    }
    
    // Escreve cabeçalho temporário (será atualizado depois)
    CabecalhoIndice novo_cabecalho = bd->cabecalho;
    novo_cabecalho.proximo_offset = sizeof(CabecalhoIndice);
    novo_cabecalho.num_paginas = 0;
    fwrite(&novo_cabecalho, sizeof(CabecalhoIndice), 1, temp_indice);
    
    // Compacta páginas recursivamente, começando pela raiz
    long novo_offset_raiz = compactar_paginas_recursivo(bd, bd->raiz_ram, temp_indice, &novo_cabecalho);
    
    fclose(temp_indice);
    fclose(bd->arquivo_indice);
    
    // Substitui arquivo de índice
    remove(ARQUIVO_INDICE);
    rename("models/indice_temp.bin", ARQUIVO_INDICE);
    
    // Reabre arquivo de índice e atualiza cabeçalho
    bd->arquivo_indice = fopen(ARQUIVO_INDICE, "r+b");
    novo_cabecalho.offset_raiz = novo_offset_raiz;
    escrever_cabecalho(bd->arquivo_indice, &novo_cabecalho);
    
    // Atualiza cabeçalho em memória
    bd->cabecalho = novo_cabecalho;
    
    // Recarrega raiz com novo offset
    free(bd->raiz_ram);
    bd->raiz_ram = ler_pagina(bd->arquivo_indice, novo_offset_raiz);
    
    free(lista.chaves);
    printf("Compactacao concluida! %d registros reorganizados.\n", lista.num_chaves);
    printf("Paginas validas: %d (altura: %d)\n\n", novo_cabecalho.num_paginas, novo_cabecalho.altura);
}

// Funções de inicialização e finalização do banco de dados
BancoDados* inicializar_banco() {
    BancoDados *bd = malloc(sizeof(BancoDados));
    
    // Abre ou cria arquivo de índice
    bd->arquivo_indice = fopen(ARQUIVO_INDICE, "r+b");
    bool indice_novo = false;
    
    if (!bd->arquivo_indice) {
        bd->arquivo_indice = fopen(ARQUIVO_INDICE, "w+b");
        indice_novo = true;
    }
    
    // Abre ou cria arquivo de dados
    bd->arquivo_dados = fopen(ARQUIVO_DADOS, "r+b");
    if (!bd->arquivo_dados) {
        bd->arquivo_dados = fopen(ARQUIVO_DADOS, "w+b");
    }
    
    if (indice_novo) {
        // Inicializa novo banco
        bd->cabecalho.offset_raiz = sizeof(CabecalhoIndice);
        bd->cabecalho.proximo_offset = sizeof(CabecalhoIndice) + sizeof(Pagina);
        bd->cabecalho.altura = 0;
        bd->cabecalho.num_paginas = 1;
        escrever_cabecalho(bd->arquivo_indice, &bd->cabecalho);
        
        // Cria raiz vazia
        bd->raiz_ram = criar_pagina(true);
        bd->raiz_ram->offset_proprio = bd->cabecalho.offset_raiz;
        escrever_pagina(bd->arquivo_indice, bd->raiz_ram, bd->raiz_ram->offset_proprio);
    } else {
        // Carrega banco existente
        ler_cabecalho(bd->arquivo_indice, &bd->cabecalho);
        bd->raiz_ram = ler_pagina(bd->arquivo_indice, bd->cabecalho.offset_raiz);
    }
    
    return bd;
}

/**
 * Finaliza o banco de dados
 */
void finalizar_banco(BancoDados *bd) {
    if (bd->raiz_ram) {
        escrever_pagina(bd->arquivo_indice, bd->raiz_ram, bd->raiz_ram->offset_proprio);
        free(bd->raiz_ram);
    }
    
    if (bd->arquivo_indice) fclose(bd->arquivo_indice);
    if (bd->arquivo_dados) fclose(bd->arquivo_dados);
    
    free(bd);
}

// Funções de interface do usuário
void inserir_multiplos_limiares(BancoDados *bd) {
    char nome_arquivo[TAM_NOME_ARQUIVO];
    printf("\nNome do arquivo PGM: ");
    scanf("%s", nome_arquivo);
    
    RegistroImagem img_original;
    if (!ler_pgm(nome_arquivo, &img_original)) {
        return;
    }
    
    int num_limiares;
    printf("Quantos limiares? ");
    scanf("%d", &num_limiares);
    
    if (num_limiares <= 0 || num_limiares > 20) {
        printf("Número inválido (1-20).\n");
        return;
    }
    
    int *limiares = malloc(num_limiares * sizeof(int));
    printf("Digite os %d limiares (0-255):\n", num_limiares);
    for (int i = 0; i < num_limiares; i++) {
        printf("  Limiar %d: ", i + 1);
        scanf("%d", &limiares[i]);
    }
    
    printf("\nProcessando...\n");
    for (int i = 0; i < num_limiares; i++) {
        RegistroImagem img_binaria;
        aplicar_limiarizacao(&img_original, &img_binaria, limiares[i]);
        
        long offset = salvar_imagem(bd->arquivo_dados, &img_binaria);
        
        Chave chave;
        strcpy(chave.nome_arquivo, nome_arquivo);
        chave.limiar = limiares[i];
        chave.offset_dados = offset;
        
        inserir(bd, &chave);
        printf("  [OK] Inserido: %s (limiar %d)\n", nome_arquivo, limiares[i]);
    }
    
    free(limiares);
    printf("\n[OK] %d imagens inseridas com sucesso!\n", num_limiares);
}

/**
 * Busca uma imagem
 */
void buscar_imagem(BancoDados *bd) {
    char nome_arquivo[TAM_NOME_ARQUIVO];
    int limiar;
    
    printf("\nNome do arquivo: ");
    scanf("%s", nome_arquivo);
    printf("Limiar: ");
    scanf("%d", &limiar);
    
    Chave chave_busca, resultado;
    strcpy(chave_busca.nome_arquivo, nome_arquivo);
    chave_busca.limiar = limiar;
    
    if (buscar(bd, &chave_busca, &resultado)) {
        printf("\n[OK] Imagem encontrada!\n");
        printf("  Arquivo: %s\n", resultado.nome_arquivo);
        printf("  Limiar: %d\n", resultado.limiar);
        printf("  Offset: %ld\n", resultado.offset_dados);
    } else {
        printf("\n[ERRO] Imagem nao encontrada.\n");
    }
}

/**
 * Remove uma imagem
 */
void remover_imagem(BancoDados *bd) {
    char nome_arquivo[TAM_NOME_ARQUIVO];
    int limiar;
    
    printf("\nNome do arquivo: ");
    scanf("%s", nome_arquivo);
    printf("Limiar: ");
    scanf("%d", &limiar);
    
    Chave chave;
    strcpy(chave.nome_arquivo, nome_arquivo);
    chave.limiar = limiar;
    
    if (remover(bd, &chave)) {
        printf("\n[OK] Imagem removida com sucesso!\n");
    } else {
        printf("\n[ERRO] Imagem nao encontrada.\n");
    }
}

/**
 * Exporta uma imagem
 */
void exportar_imagem(BancoDados *bd) {
    char nome_arquivo[TAM_NOME_ARQUIVO];
    char nome_saida[TAM_NOME_ARQUIVO];
    int limiar, formato;
    
    printf("\nNome do arquivo no banco: ");
    scanf("%s", nome_arquivo);
    printf("Limiar: ");
    scanf("%d", &limiar);
    printf("Nome do arquivo de saida: ");
    scanf("%s", nome_saida);
    printf("Formato de saida (1=P2 ASCII, 2=P5 Binario): ");
    scanf("%d", &formato);
    
    Chave chave_busca, resultado;
    strcpy(chave_busca.nome_arquivo, nome_arquivo);
    chave_busca.limiar = limiar;
    
    if (buscar(bd, &chave_busca, &resultado)) {
        RegistroImagem img;
        if (carregar_imagem(bd->arquivo_dados, resultado.offset_dados, &img)) {
            bool formato_p2 = (formato == 1);
            if (exportar_pgm(&img, nome_saida, formato_p2)) {
                printf("\n[OK] Imagem exportada para %s (formato %s)\n", 
                       nome_saida, formato_p2 ? "P2" : "P5");
            }
        }
    } else {
        printf("\n[ERRO] Imagem nao encontrada.\n");
    }
}

/**
 * Exibe estatísticas
 */
void exibir_estatisticas(BancoDados *bd) {
    printf("\n=== Estatísticas da Árvore-B ===\n");
    printf("Ordem: %d\n", ORDEM);
    printf("Altura: %d\n", bd->cabecalho.altura);
    printf("Número de páginas: %d\n", bd->cabecalho.num_paginas);
    printf("Offset da raiz: %ld\n", bd->cabecalho.offset_raiz);
    printf("Chaves na raiz: %d\n", bd->raiz_ram->num_chaves);
    printf("Raiz é folha: %s\n", bd->raiz_ram->eh_folha ? "SIM" : "NÃO");
    printf("================================\n");
}

/**
 * Exibe informações do sistema
 */
void exibir_informacoes() {
    printf("\n===================================================\n");
    printf("        INFORMACOES DO SISTEMA\n");
    printf("===================================================\n");
    printf("\n[PROJETO]\n");
    printf("  Titulo: Arvore-B de Ordem 3 para Banco de Imagens\n");
    printf("  Descricao: Sistema de indexacao de imagens PGM\n");
    printf("             usando Arvore-B com multiplos limiares\n");
    printf("\n[ACADEMICO]\n");
    printf("  Disciplina: Estruturas de Dados II\n");
    printf("  Professor: Emilio Bergamim Junior\n");
    printf("  Aluno: Luiz Gustavo Damas\n");
    printf("\n[TECNOLOGIAS]\n");
    printf("  Linguagem: C (padrao C11)\n");
    printf("  Compilador: GCC\n");
    printf("  Estrutura: Arvore-B de ordem 3\n");
    printf("  Formato: Arquivos binarios (.bin)\n");
    printf("  Imagens: PGM (P2 ASCII e P5 binario)\n");
    printf("\n[CARACTERISTICAS]\n");
    printf("  - Raiz virtualizada (sempre em RAM)\n");
    printf("  - Insercao multipla de limiares\n");
    printf("  - Remocao fisica de chaves\n");
    printf("  - Compactacao de dados\n");
    printf("  - Suporte a imagens ate 640x480 pixels\n");
    printf("===================================================\n\n");
}

/**
 * Menu principal
 */
void exibir_menu() {
    printf("\n===============================================\n");
    printf("   ARVORE-B DE ORDEM 3 - BANCO DE IMAGENS\n");
    printf("===============================================\n");
    printf(" 1. Inserir imagem (multiplos limiares)\n");
    printf(" 2. Buscar imagem\n");
    printf(" 3. Remover imagem\n");
    printf(" 4. Percurso em ordem (listar)\n");
    printf(" 5. Visualizar paginas (debug)\n");
    printf(" 6. Exportar imagem\n");
    printf(" 7. Compactar arquivo de dados\n");
    printf(" 8. Estatisticas\n");
    printf(" 9. Informacoes do sistema\n");
    printf(" 0. Sair\n");
    printf("===============================================\n");
    printf("Opcao: ");
}

// main function    
int main() {
    printf("===================================================\n");
    printf("  Arvore-B Paginada de Ordem 3\n");
    printf("  Banco de Dados de Imagens\n");
    printf("===================================================\n\n");
    
    printf("Inicializando banco de dados...\n");
    BancoDados *bd = inicializar_banco();
    printf("[OK] Banco de dados pronto!\n");
    
    int opcao;
    do {
        exibir_menu();
        scanf("%d", &opcao);
        
        switch(opcao) {
            case 1:
                inserir_multiplos_limiares(bd);
                break;
            case 2:
                buscar_imagem(bd);
                break;
            case 3:
                remover_imagem(bd);
                break;
            case 4:
                percurso_em_ordem(bd);
                break;
            case 5:
                visualizar_paginas(bd);
                break;
            case 6:
                exportar_imagem(bd);
                break;
            case 7:
                compactar(bd);
                break;
            case 8:
                exibir_estatisticas(bd);
                break;
            case 9:
                exibir_informacoes();
                break;
            case 0:
                printf("\nEncerrando...\n");
                break;
            default:
                printf("\n[ERRO] Opcao invalida!\n");
        }
    } while(opcao != 0);
    
    finalizar_banco(bd);
    printf("[OK] Banco de dados fechado. Ate logo!\n\n");
    
    return 0;
}
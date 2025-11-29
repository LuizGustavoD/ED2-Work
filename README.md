# Árvore-B Paginada de Ordem 3 - Banco de Dados de Imagens

## Descrição

Implementação completa de uma **Árvore-B paginada de ordem 3** para indexação eficiente de banco de dados de imagens binárias geradas a partir de arquivos PGM (Portable GrayMap).

## Características Principais

### 1. Estrutura da Árvore-B
- **Ordem 3**: Máximo 2 chaves por nó, 3 filhos
- **Raiz virtualizada**: Sempre mantida em RAM para otimização
- **Arquivos binários**: Índice e dados separados
- **Ordenação**: Por nome de arquivo e limiar

### 2. Funcionalidades Implementadas

✅ **Inserção (INSERT)**
- Inserção de chaves com split automático de nós
- Suporte a múltiplos limiares em uma única operação
- Balanceamento automático da árvore

✅ **Remoção (DELETE)**
- Remoção física de chaves (não apenas marcação)
- Redistribuição e merge de nós
- Manutenção do balanceamento

✅ **Busca (SEARCH)**
- Busca eficiente com complexidade O(log n)
- Raiz em RAM reduz acessos a disco

✅ **Percurso Ordenado**
- Listagem de todas as chaves em ordem crescente
- Percurso in-order recursivo

✅ **Visualização de Páginas**
- Função de debug para inspecionar páginas
- Exibe conteúdo completo do arquivo de índice

✅ **Compactação**
- Remove fragmentação do arquivo de dados E índice
- Utiliza percurso ordenado
- Reconstrói ambos os arquivos (dados + índice)
- Remove páginas inválidas e vazias

## Estrutura de Dados

### Chave
```c
typedef struct {
    char nome_arquivo[256];  // Nome do arquivo PGM
    int limiar;              // Limiar aplicado (0-255)
    long offset_dados;       // Posição no arquivo de dados
} Chave;
```

### Página (Nó da Árvore-B)
```c
typedef struct {
    int num_chaves;              // Número de chaves (0-2)
    Chave chaves[2];             // Array de chaves
    long filhos[3];              // Offsets dos filhos
    bool eh_folha;               // Indica se é folha
    long offset_proprio;         // Posição no arquivo
} Pagina;
```

## Compilação

### Usando Scripts Automatizados (Recomendado)

A forma mais fácil é usar os scripts na pasta `scripts/`:

**Windows PowerShell:**
```powershell
.\scripts\compilar.ps1
```

**Windows CMD:**
```cmd
scripts\compilar.cmd
```

**Linux/Mac:**
```bash
chmod +x scripts/compilar.sh  # Primeira vez apenas
./scripts/compilar.sh
```

> 📖 Para mais detalhes, veja [scripts/README.md](scripts/README.md)

### Compilação Manual

**Windows:**
```bash
gcc -Wall -Wextra -std=c11 -O2 -o arvore_b.exe arvore_b.c
```

**Linux/Mac:**
```bash
gcc -Wall -Wextra -std=c11 -O2 -o arvore_b arvore_b.c
```

### Usando Make
```bash
make
```

## Execução

```bash
./arvore_b          # Linux/Mac
arvore_b.exe        # Windows
```

## Menu de Opções

```
1. Inserir imagem (múltiplos limiares)
2. Buscar imagem
3. Remover imagem
4. Percurso em ordem (listar)
5. Visualizar páginas (debug)
6. Exportar imagem
7. Compactar arquivo de dados
8. Estatísticas
9. Informações do sistema
0. Sair
```

### Descrição Detalhada

**1. Inserir imagem (múltiplos limiares)**
- Lê arquivo PGM (P2 ou P5)
- Aplica N limiares de binarização
- Insere todas as versões no banco

**2. Buscar imagem**
- Busca por nome e limiar
- Retorna informações do registro

**3. Remover imagem**
- Remove fisicamente da árvore
- Redistribuição e merge automáticos
- Reduz altura se necessário

**4. Percurso em ordem (listar)**
- Exibe todas as chaves ordenadas
- In-order traversal

**5. Visualizar páginas (debug)**
- Mostra conteúdo de cada página
- Útil para análise e debug
- Exibe páginas inválidas também

**6. Exportar imagem**
- Recupera imagem do banco
- Escolha de formato: P2 (ASCII) ou P5 (binário)
- Salva em arquivo PGM

**7. Compactar arquivo de dados**
- Remove fragmentação de dados E índice
- Reorganiza ambos os arquivos
- Remove páginas inválidas

**8. Estatísticas**
- Altura, páginas, ordem, offset da raiz
- Informações sobre a raiz

**9. Informações do sistema**
- Professor, aluno, tecnologias usadas

## Exemplo de Uso

### 1. Inserir Imagem com Múltiplos Limiares
```
Opção: 1
Nome do arquivo PGM: foto.pgm
Quantos limiares? 3
  Limiar 1: 100
  Limiar 2: 150
  Limiar 3: 200

Resultado: 3 versões binárias inseridas
```

### 2. Listar Todas as Imagens
```
Opção: 4

Saída:
  foto.pgm, limiar=100 (offset: 0)
  foto.pgm, limiar=150 (offset: 262144)
  foto.pgm, limiar=200 (offset: 524288)
```

### 3. Visualizar Páginas (Debug)
```
Opção: 5

Saída:
Página [0]: (offset: 24, folha: SIM, chaves: 2)
  Chave [foto.pgm], limiar=[100], offset_dados=0
  Chave [foto.pgm], limiar=[150], offset_dados=262144
```

## Arquivos Gerados

- **models/indice.bin**: Arquivo binário com a estrutura da Árvore-B
- **models/dados.bin**: Arquivo binário com as imagens

## Formato PGM Suportado

### P2 (ASCII)
```
P2
largura altura
max_valor
pixel1 pixel2 pixel3 ...
```

### P5 (Binário)
```
P5
largura altura
max_valor
[dados binários]
```

## Complexidade das Operações

| Operação | Complexidade Temporal | Acessos a Disco |
|----------|----------------------|-----------------|
| Busca | O(log n) | altura - 1 |
| Inserção | O(log n) | altura |
| Remoção | O(log n) | altura * 2 |
| Percurso | O(n) | n páginas |
| Compactação | O(n) | 2n (dados + índice) |

## Características Técnicas

### Ordem da Árvore: 3
- Mínimo de chaves (exceto raiz): 1
- Máximo de chaves: 2
- Mínimo de filhos: 2
- Máximo de filhos: 3

### Virtualização da Raiz
- Raiz sempre em RAM
- Reduz 1 acesso a disco por operação
- Sincronização automática

### Compactação Inteligente
- Arquivo de dados E índice são compactados
- Remove páginas inválidas (num_chaves < 0) e vazias
- Reorganiza sequencialmente
- Usa percurso ordenado para coletar chaves válidas

## Limitações

- Tamanho máximo de imagem: 640x480 pixels
- Nome do arquivo: máximo 256 caracteres
- Valores de pixel: 0-255 (8 bits)
- Ordem fixa: 3 (não configurável)

## Estrutura do Código

```
arvore_b.c (1500+ linhas)
├── Definições e estruturas (linhas 1-100)
├── Funções auxiliares (linhas 100-250)
├── I/O de páginas (linhas 250-350)
├── Busca (linhas 350-450)
├── Inserção (linhas 450-650)
├── Remoção (linhas 650-1050)
├── Percurso (linhas 1050-1150)
├── Visualização (linhas 1150-1250)
├── Gerenciamento PGM (linhas 1250-1350)
├── Compactação (linhas 1350-1450)
└── Interface/Menu (linhas 1450-1550)
```

## Autor

**Professor:** Emilio Bergamim Junior  
**Aluno:** Luiz Gustavo Damas  
**Disciplina:** Estruturas de Dados II

## Licença

Código educacional - livre para uso acadêmico

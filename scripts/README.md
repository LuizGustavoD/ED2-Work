# Scripts de Compilação

Este diretório contém scripts de compilação para diferentes sistemas operacionais.

## 📜 Scripts Disponíveis

### 1. **compilar.ps1** (Windows PowerShell)

Script para compilação no Windows usando PowerShell.

**Como usar:**
```powershell
.\compilar.ps1
```

**Requisitos:**
- Windows 10/11
- PowerShell 5.1 ou superior
- GCC instalado (MinGW ou similar)

**Recursos:**
- Saída colorida (verde para sucesso, vermelho para erro)
- Detecção automática de erros
- Instruções de execução após compilar

---

### 2. **compilar.sh** (Linux/Mac)

Script para compilação em sistemas Unix-like (Linux, macOS, BSD).

**Como usar:**
```bash
# Dar permissão de execução (primeira vez)
chmod +x compilar.sh

# Executar
./compilar.sh
```

**Requisitos:**
- Linux, macOS ou outro sistema Unix
- Bash shell
- GCC instalado

**Recursos:**
- Saída colorida com códigos ANSI
- Detecção de erros com exit code
- Compatível com terminais modernos

---

### 3. **compilar.cmd** (Windows CMD)

Script para compilação no Windows usando Prompt de Comando (CMD).

**Como usar:**
```cmd
compilar.cmd
```

ou simplesmente:
```cmd
compilar
```

**Requisitos:**
- Windows (qualquer versão)
- GCC instalado (MinGW ou similar)

**Recursos:**
- Compatível com CMD tradicional
- Detecção de erros via ERRORLEVEL
- Funcionamento garantido em Windows antigos

---

## 🔧 Compilação Manual

Se preferir compilar manualmente sem usar os scripts:

### Windows
```bash
gcc -Wall -Wextra -std=c11 -O2 -o arvore_b.exe arvore_b.c
```

### Linux/Mac
```bash
gcc -Wall -Wextra -std=c11 -O2 -o arvore_b arvore_b.c
```

---

## 🚀 Após Compilar

### Windows
```powershell
.\arvore_b.exe
```

### Linux/Mac
```bash
./arvore_b
```

---

## ⚙️ Flags de Compilação

| Flag | Descrição |
|------|-----------|
| `-Wall` | Habilita todos os warnings comuns |
| `-Wextra` | Habilita warnings extras |
| `-std=c11` | Usa o padrão C11 |
| `-O2` | Otimização nível 2 |
| `-o <arquivo>` | Nome do executável de saída |

---

## 📋 Troubleshooting

### "gcc não é reconhecido como comando"

**Windows:**
- Instale MinGW-w64 ou MSYS2
- Adicione o GCC ao PATH do sistema

**Linux:**
```bash
sudo apt install build-essential  # Debian/Ubuntu
sudo yum install gcc              # RedHat/CentOS
```

**macOS:**
```bash
xcode-select --install
```

### Erro de permissão (Linux/Mac)

```bash
chmod +x compilar.sh
```

### Script não executa no PowerShell

```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

---

## 📝 Observações

- Todos os scripts geram o mesmo executável
- O executável gerado varia por plataforma:
  - **Windows:** `arvore_b.exe`
  - **Linux/Mac:** `arvore_b`
- Os scripts criam a pasta `models/` automaticamente se necessário
- Certifique-se de estar no diretório raiz do projeto ao executar

---

## 🆘 Suporte

Em caso de problemas:
1. Verifique se o GCC está instalado: `gcc --version`
2. Certifique-se de estar no diretório correto
3. Verifique se o arquivo `arvore_b.c` existe
4. Consulte o README.md principal do projeto

# Makefile para Árvore-B de Ordem 3

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -g
TARGET = arvore_b
SOURCE = arvore_b.c

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SOURCE)
	@echo Compilando $(SOURCE)...
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)
	@echo Compilacao concluida!

clean:
	@echo Limpando arquivos...
	@if exist $(TARGET).exe del /Q $(TARGET).exe 2>nul
	@if exist indice.dat del /Q indice.dat 2>nul
	@if exist dados.dat del /Q dados.dat 2>nul
	@if exist dados_temp.dat del /Q dados_temp.dat 2>nul
	@echo Limpeza concluida!

run: $(TARGET)
	./$(TARGET)

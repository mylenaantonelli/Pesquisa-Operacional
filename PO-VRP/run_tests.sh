#!/bin/bash

# Leitura dos argumentos
ITERATIONS=$1
OUTPUT_FILE=$2

# Verifica se os argumentos foram fornecidos
if [ -z "$ITERATIONS" ] || [ -z "$OUTPUT_FILE" ]; then
    echo "ERRO: Parâmetros faltando."
    echo "Uso: ./run_tests.sh [NumeroDeIteracoes] [ArquivoDeSaida.txt]"
    echo "Exemplo: ./run_tests.sh 2000 relatorio_2000_iters.txt"
    exit 1
fi

echo "Iniciando testes em lote..."
echo "Iteracoes por instancia: $ITERATIONS"
echo "Salvando em: $OUTPUT_FILE"
echo "----------------------------------------" > "$OUTPUT_FILE"
echo "----------------------------------------" >> "$OUTPUT_FILE"

# Loop para processar os arquivos .txt
for f in *.txt; do
    # Verifica se o arquivo nao e o arquivo de saida
    if [ "$f" != "$OUTPUT_FILE" ]; then
        echo "Processando arquivo: $f"
        echo "----------------------------------------" >> "$OUTPUT_FILE"
        echo "INSTANCIA: $f" >> "$OUTPUT_FILE"
        
        # Aqui, o nome do executável deve ser exatamente vrptw_solver.exe
        # E ele deve ser chamado com o caminho completo, se necessario, ou com ./
        ./vrptw_solver.exe "$f" "$ITERATIONS" >> "$OUTPUT_FILE"
        
        echo "Concluido: $f"
    fi
done

echo
echo "Todos os testes finalizaram! Verifique o arquivo $OUTPUT_FILE"

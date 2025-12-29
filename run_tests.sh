#!/bin/bash

ITERATIONS=$1
OUTPUT_FILE=$2
INST_DIR="instancias"

if [ -z "$ITERATIONS" ] || [ -z "$OUTPUT_FILE" ]; then
    echo "ERRO: Parâmetros faltando."
    echo "Uso: ./run_tests.sh [Iteracoes] [ArquivoSaida.txt]"
    exit 1
fi

# Verifica se a pasta de instâncias existe
if [ ! -d "$INST_DIR" ]; then
    echo "ERRO: Pasta '$INST_DIR' não encontrada."
    exit 1
fi

echo "Iniciando testes em lote..."
echo "----------------------------------------" > "$OUTPUT_FILE"

for f in "$INST_DIR"/*.txt; do
    echo "Processando: $f"
    echo "----------------------------------------" >> "$OUTPUT_FILE"
    echo "INSTANCIA: $f" >> "$OUTPUT_FILE"

    ./bin/vrptw_solver "$f" "$ITERATIONS" >> "$OUTPUT_FILE"

    echo "Concluido: $f"
done

echo "Todos os testes finalizaram!"


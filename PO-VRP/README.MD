Solver VRPTW com Large Neighborhood Search (LNS)
Este projeto implementa uma meta-heurística LNS para resolver o Problema de Roteamento de Veículos com Janelas de Tempo (VRPTW), utilizando instâncias de benchmark do padrão Solomon.

📋 Pré-requisitos
Compilador C++ compatível com C++11 ou superior (ex: g++, clang).

Sistema Operacional: Windows, Linux ou macOS.

Como Compilar
Abra o terminal na pasta do arquivo e execute:
g++ main.cpp -o vrptw_solver -O3
-O3: Ativa otimizações de performance (essencial para meta-heurísticas).

Como Rodar
A execução requer o caminho do arquivo da instância e, opcionalmente, o número de iterações.

Sintaxe:
./vrptw_solver <caminho_instancia.txt> [numero_iteracoes]
Exemplo:
./vrptw_solver c101.txt 2000

Parâmetros e Configuração
Os principais parâmetros da meta-heurística estão definidos no código para facilitar o ajuste:

Iterações (Argumento de Linha de Comando): Define quanto tempo o algoritmo vai rodar.

Padrão: 1000.

Sugestão: Aumentar para 5000 ou 10000 melhora a qualidade da solução (reduz o custo), mas aumenta o tempo de execução linearmente.

Perturbation Size (Variável perturbation_size):

Define quantos clientes são removidos e reinseridos a cada iteração.

Valor Atual: 15.

Impacto: Valores maiores permitem "pulos" maiores no espaço de busca (bom para diversificação), mas tornam a reconstrução mais lenta.

Metodologia e Heurística
O algoritmo segue a estrutura Destroy and Repair:
Solução Inicial: Utiliza uma heurística construtiva gulosa (Nearest Neighbor) validando janelas de tempo.
Operador de Destruição (Destroy): Remove $N$ clientes aleatoriamente das rotas existentes.
Operador de Reparo (Repair): Utiliza Best Insertion. Tenta reinserir cada cliente removido na posição que causa o menor aumento no custo total, respeitando todas as restrições (Capacidade e Janelas de Tempo).
Critério de Aceitação: A nova solução é aceita se seu custo for menor que a solução atual (Hill Climbing).

Formato das Instâncias
O solver aceita arquivos no formato clássico de Solomon (100 clientes), contendo coordenadas, demanda, janelas de tempo e tempo de serviço.
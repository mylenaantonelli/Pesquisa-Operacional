# VRPTW — Large Neighborhood Search (LNS)

Este projeto implementa uma **meta-heurística Large Neighborhood Search (LNS)** para resolver o **Problema de Roteamento de Veículos com Janelas de Tempo (VRPTW)**, utilizando instâncias clássicas do benchmark de **Solomon**.

Desenvolvido em C++, o foco desta implementação foi construir uma lógica rigorosa que assegura o cumprimento de todas as restrições do problema. O código garante que os veículos operem sempre dentro do seu limite de capacidade e que cada cliente seja atendido estritamente dentro da sua janela de tempo programada. Com essa estrutura de validação, o algoritmo explora rotas eficientes por meio de ciclos de 'desconstrução e reconstrução' da solução, permitindo uma comparação direta entre os resultados obtidos e os melhores valores registrados na literatura.


## Instâncias Utilizadas

As instâncias utilizadas estão localizadas na pasta `instancias/` e seguem o **formato Solomon VRPTW**, contendo:

- Coordenadas dos clientes
- Demanda de cada cliente
- Janelas de tempo (`ready_time` e `due_date`)
- Tempo de serviço
- Capacidade do veículo

Exemplos de instâncias:
- `C101.txt`, `C102.txt` — clientes agrupados
- `R101.txt`, `R102.txt` — clientes distribuídos aleatoriamente
- `RC101.txt` — instâncias mistas

O cliente `0` representa o **depósito**.

---

## Compilação

### Linux

```bash
g++ -O2 -std=c++17 src/vrptw_solver.cpp -o bin/vrptw_solver 
```

### Windows
```bash
g++ -O2 -std=c++17 src/vrptw_solver.cpp -o bin/vrptw_solver.exe
```

## Execução - Uma instância por vez
Este modo permite executar o algoritmo manualmente para uma instância específica.

### Linux

```bash
./bin/vrptw_solver instancias/C101.txt 2000
```

### Windows
```bash
bin\vrptw_solver.exe instancias\C101.txt 2000
```
### Parâmetros
```css
vrptw_solver <arquivo_instancia> [iteracoes]
```
* arquivo_instancia: caminho para o arquivo da instância
* iteracoes: número de iterações do algoritmo LNS

## Execução em Lote com Scripts
Para facilitar testes em larga escala, o projeto já disponibiliza scripts de execução automática.

### Linux - run_tests.sh

```bash
chmod +x run_tests.sh
./run_tests.sh 1000 resultados/resultados_linux.txt
```
* 1000: número de iterações
* resultados/resultados_linux.txt: arquivo de saída com os resultados

### Windows - run_tests.bat

```bash
run_tests.bat 2000 resultados\resultados_windows.txt
```
* 2000: número de iterações
* resultados\resultados_windows.txt: arquivo de saída com os resultados

Os scripts .sh e .bat possuem lógica equivalente, garantindo consistência entre os ambientes Linux e Windows.

## Meta-heurística (LNS)
O algoritmo segue as seguintes etapas:

**1. Geração de solução inicial**  
Utiliza uma heurística gulosa baseada no vizinho mais próximo, garantindo viabilidade desde o início.

**2. Destruição (Destroy Operator)**  
Remove aleatoriamente um subconjunto de clientes da solução atual, promovendo diversificação.

**3. Reparo (Repair Operator)**  
Reinsere os clientes removidos utilizando inserção gulosa de menor custo, respeitando todas as restrições.

**4. Critério de Aceitação**  
Apenas soluções que apresentam melhora no custo total são aceitas.

## Parâmetros do Algoritimo

### Número de Iterações
Define quantas vezes o ciclo de destruição e reparo é executado.

Valores testados:
* 1000
* 2000
* 10000

Justificativa:
O valor 2000 iterações apresentou um bom equilíbrio entre qualidade da solução e tempo de execução.

### Tamanho da Destruição (LNS)
```cpp
int destroy_size = 15;
```
Define quantos clientes são removidos em cada iteração do LNS.

Justificativa:
Esse valor permite escapar de ótimos locais sem comprometer excessivamente o processo de reconstrução da solução.

## Escolha dos Parâmetros

Escolhemos os parâmetros do algoritmo com base em referências da área e em testes que fizemos durante o projeto. Tentamos equilibrar o tempo de processamento com a precisão dos resultados.


### Número de Iterações

O número de iterações define quantas vezes o algoritmo aplica os operadores de destruição e reconstrução da solução no processo de busca.

Neste trabalho, começamos com 1000 iterações, porque nos nossos primeiros testes, esse valor entregou resultados muito bons sem demorar muito.

Além disso, também testamos com **2000** e **10000 iterações** e vimos que a qualidade da solução tende a melhorar à medida que aumentamos o número de iterações, mas o tempo de execução também cresce.

Dessa forma, mantivemos as 1000 iterações para o uso geral e deixamos os valores maiores apenas para análise comparativa.


### Tamanho da Destruição

Esse parâmetro decide quantos clientes vamos tirar da rota atual em cada rodada para tentar algo novo. É ele que controla o quanto o algoritmo vai variar o caminho.

Definimos esse valor como uma parte do total de clientes da instância, porque percebemos o seguinte:

* Se tirarmos poucos clientes, o algoritmo quase não muda nada e acaba não explorando caminhos novos.
* Se tirarmos clientes demais, a gente acaba destruindo partes boas da rota que já tínhamos encontrado.
* Um valor meio-termo foi o que funcionou melhor, equilibrando bem a busca por novas opções sem perder o que já estava bom.


### Solução Inicial

Para começar, a gente cria uma solução inicial básica, mas que já respeita todas as regras de capacidade dos veículos e janelas de tempo.

Optamos por começar com algo simples porque:

* O algoritmo inicia muito mais rápido, sem perder tempo no começo;
* Assim, deixamos o trabalho de melhorar os resultados para o LNS ao longo das iterações.


## Exemplo de Saída do Programa

A seguir é apresentado um **exemplo de saída** gerada pelo algoritmo ao executar a instância `C101.txt` com **1000 iterações** da meta-heurística LNS.

```text
INSTANCIA: instancias/C101.txt
--- INICIANDO LNS PARA VRPTW ---
Instancia: instancias/C101.txt
Iteracoes: 1000
Solucao Inicial: 1870.69 | Veiculos: 21
Iter 0: Melhor = 1803.52
Iter 1: Melhor = 1665.83
Iter 2: Melhor = 1596.46
Iter 3: Melhor = 1478.65
Iter 4: Melhor = 1433.74
...
Iter 78: Melhor = 897.47

=== RESULTADO FINAL ===
Custo Total: 897.47
Veiculos Usados: 11
Tempo: 0.11 s

Rota 1: 0 -> 20 -> 24 -> 25 -> 27 -> 29 -> 22 -> 21 -> 0
Rota 2: 0 -> 5 -> 3 -> 7 -> 8 -> 15 -> 16 -> 14 -> 12 -> 0
...
Rota 11: 0 -> 81 -> 78 -> 76 -> 71 -> 70 -> 73 -> 77 -> 79 -> 80 -> 0
```

### Interpretação da Saída

* Solução Inicial: custo e número de veículos obtidos pela heurística construtiva inicial.  
* Iter i: Melhor = X: indica a melhoria evolutiva do custo total ao longo das iterações do LNS.  
* Custo Total: soma das distâncias de todas as rotas da melhor solução encontrada.  
* Veículos Usados: número total de rotas (veículos) necessárias para atender todos os clientes.  
* Tempo: tempo total de execução do algoritmo.  
* Rotas: sequência de clientes atendidos por cada veículo, iniciando e finalizando no depósito (cliente 0).  
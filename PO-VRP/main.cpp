/**
 * VRPTW Solver usando Large Neighborhood Search (LNS)
 * Autor: Gemini (Assistente AI)
 * Descrição: Resolve o Problema de Roteamento de Veículos com Janelas de Tempo
 * utilizando destruição e reparo (LNS).
 */

 #include <iostream>
 #include <fstream>
 #include <vector>
 #include <cmath>
 #include <iomanip>
 #include <limits>
 #include <algorithm>
 #include <chrono>
 #include <random>
 #include <string>
 
 using namespace std;
 
 // --- ESTRUTURAS DE DADOS ---
 
 struct Customer {
     int id;
     double x, y;
     double demand;
     double ready_time;
     double due_date;
     double service_time;
 };
 
 struct Route {
     vector<int> path;
     double total_dist;
     double current_load;
     bool feasible;
 };
 
 // --- CONFIGURAÇÕES GERAIS ---
 
 const double BIG_DOUBLE = 1e9;
 const int MAX_VEHICLES = 25;
 double VEHICLE_CAPACITY = 200.0;
 
 vector<Customer> customers;
 vector<vector<double>> dist_matrix;
 int num_customers;
 
 // --- FUNÇÕES AUXILIARES ---
 
 double calc_dist(int i, int j) {
     double dx = customers[i].x - customers[j].x;
     double dy = customers[i].y - customers[j].y;
     return sqrt(dx*dx + dy*dy);
 }
 
 // ======================================================================
 // === VERIFICAÇÃO DE VIABILIDADE (CRUCIAL PARA VRPTW) ===================
 // ======================================================================
 
 // Verifica se uma rota é viável, respeitando capacidade e janelas de tempo.
 bool is_valid_route(const vector<int>& path, double& out_dist) {
     if (path.empty()) return true;
 
     double load = 0;
     double time = 0;
     double dist = 0;
     int curr = 0; // Nó atual: Depósito (ID 0)
 
     // O veículo sai do depósito no Ready Time dele (0 ou o tempo pronto)
     time = max(customers[0].ready_time, time);
 
     for (int next_node : path) {
         // 1. Restrição de Capacidade
         load += customers[next_node].demand;
         if (load > VEHICLE_CAPACITY) return false; // Violação da capacidade
 
         // 2. Tempo de Viagem
         double travel = dist_matrix[curr][next_node];
         dist += travel;
         time += travel; // Tempo de Chegada
 
         // 3. Restrição de Janela de Tempo (Due Date - limite superior)
         if (time > customers[next_node].due_date) return false; // Chegou atrasado!
         
         // 4. Espera (Waiting Time)
         // Se o tempo de chegada for menor que o Ready Time (limite inferior), o veículo espera.
         time = max(time, customers[next_node].ready_time);
 
         // 5. Tempo de Serviço
         time += customers[next_node].service_time; // Tempo em que o veículo sai do cliente
         curr = next_node;
     }
 
     // 6. Retorno ao Depósito
     double return_trip = dist_matrix[curr][0];
     dist += return_trip;
     time += return_trip;
 
     // 7. Restrição de Janela de Tempo do Depósito
     if (time > customers[0].due_date) return false; // Retornou ao depósito fora do prazo
 
     out_dist = dist; // Retorna o custo (distância) da rota
     return true;
 }
 
 double get_solution_cost(const vector<vector<int>>& solution) {
     double total = 0;
     for (const auto& route : solution) {
         double d = 0;
         // Itera sobre todas as rotas e soma os custos. A viabilidade é garantida
         // pela função is_valid_route (embora o LNS foque apenas no custo aqui).
         if (!route.empty()) {
             is_valid_route(route, d);
             total += d;
         }
     }
     return total; // Retorna o custo total (distância) da solução
 }
 
 // ======================================================================
 // === PARSER SOLOMON CORRIGIDO =========================================
 // ======================================================================
 
 void load_instance(string filename) {
     // ... (função de carregamento da instância) ...
     ifstream file(filename);
     if (!file.is_open()) {
         cerr << "Erro ao abrir arquivo: " << filename << endl;
         exit(1);
     }
 
     string line;
 
     // Lê nome da instância
     getline(file, line);
 
     // Procura linha que contém "CAPACITY"
     while (getline(file, line)) {
         if (line.find("CAPACITY") != string::npos)
             break;
     }
 
     // Lê número de veículos e capacidade
     int dummy;
     file >> dummy >> VEHICLE_CAPACITY;
 
     // Procura "CUSTOMER"
     while (getline(file, line)) {
         if (line.find("CUSTOMER") != string::npos)
             break;
     }
 
     // Pula a linha de cabeçalhos
     getline(file, line);
 
     // Agora lê os clientes
     Customer c;
     while (file >> c.id >> c.x >> c.y >> c.demand
                 >> c.ready_time >> c.due_date >> c.service_time) {
         customers.push_back(c);
     }
 
     num_customers = customers.size();
 
     if (num_customers == 0) {
         cerr << "ERRO FATAL: Nenhum cliente carregado. Verifique formato Solomon." << endl;
         exit(1);
     }
 
     // Matriz de distâncias
     dist_matrix.resize(num_customers, vector<double>(num_customers));
     for (int i = 0; i < num_customers; i++)
         for (int j = 0; j < num_customers; j++)
             dist_matrix[i][j] = calc_dist(i, j);
 }
 
 // ======================================================================
 // === SOLUÇÃO INICIAL (GULOSA/CONSTRUTIVA) ==============================
 // ======================================================================
 
 // Gera uma solução inicial viável, roteando clientes para a rota que
 // resulte no menor custo de viagem local (heurística do vizinho mais próximo).
 vector<vector<int>> generate_initial_solution() {
     vector<vector<int>> solution;
     vector<bool> visited(num_customers, false);
     visited[0] = true; // Depósito (0) sempre visitado
     int visited_count = 1;
 
     while (visited_count < num_customers) {
         vector<int> current_route;
         int curr_node = 0; // Inicia no depósito (0)
 
         while (true) {
             int best_cand = -1;
             double min_cost = BIG_DOUBLE;
 
             // Tenta adicionar o vizinho não visitado com o menor custo de viagem
             for (int i = 1; i < num_customers; i++) {
                 if (!visited[i]) {
                     // 1. Tenta inserção (temporariamente)
                     current_route.push_back(i);
                     double d;
                     
                     // 2. Verifica viabilidade (capacidade e janelas de tempo)
                     if (is_valid_route(current_route, d)) {
                         // 3. Critério Guloso: Escolhe o vizinho mais próximo viável
                         if (dist_matrix[curr_node][i] < min_cost) {
                             min_cost = dist_matrix[curr_node][i];
                             best_cand = i;
                         }
                     }
                     // Remove a tentativa para o próximo loop
                     current_route.pop_back(); 
                 }
             }
 
             if (best_cand != -1) {
                 // Adiciona o melhor candidato (o mais próximo e viável)
                 current_route.push_back(best_cand);
                 visited[best_cand] = true;
                 visited_count++;
                 curr_node = best_cand;
             } else break; // Nenhum cliente viável restante para esta rota
         }
         solution.push_back(current_route); // Finaliza a rota e adiciona à solução
     }
 
     return solution;
 }
 
 // ======================================================================
 // === OPERADOR DE DESTRUIÇÃO (LNS - REMOÇÃO) ============================
 // ======================================================================
 
 // Remove um número específico de clientes da solução atual de forma aleatória.
 void destroy_route(vector<vector<int>>& sol, vector<int>& removed_customers, int num_to_remove) {
     random_device rd;
     mt19937 gen(rd());
 
     for (int k = 0; k < num_to_remove; k++) {
         vector<int> nonEmptyRoutes;
         for (int i = 0; i < sol.size(); i++)
             if (!sol[i].empty()) nonEmptyRoutes.push_back(i);
 
         if (nonEmptyRoutes.empty()) break;
 
         // Seleção Aleatória de Rota
         uniform_int_distribution<> dis_r(0, nonEmptyRoutes.size() - 1);
         int r_idx = nonEmptyRoutes[dis_r(gen)];
 
         // Seleção Aleatória de Cliente dentro da Rota
         uniform_int_distribution<> dis_c(0, sol[r_idx].size() - 1);
         int c_idx = dis_c(gen);
 
         // Remoção
         removed_customers.push_back(sol[r_idx][c_idx]);
         sol[r_idx].erase(sol[r_idx].begin() + c_idx);
     }
 
     // Remove rotas que ficaram vazias após a destruição
     sol.erase(remove_if(sol.begin(), sol.end(),
                         [](const vector<int>& r){ return r.empty(); }),
               sol.end());
 }
 
 // ======================================================================
 // === OPERADOR DE REPARO (LNS - INSERÇÃO GULOSA) ========================
 // ======================================================================
 
 // Insere os clientes removidos de volta na solução usando a heurística
 // gulosa de melhor ajuste (best fit greedy insertion).
 void repair_route(vector<vector<int>>& sol, vector<int>& removed_customers) {
     random_device rd;
     mt19937 gen(rd());
     // 1. Embaralha a ordem de inserção dos clientes (diversidade)
     shuffle(removed_customers.begin(), removed_customers.end(), gen);
 
     for (int cust : removed_customers) {
         double best_increase = BIG_DOUBLE;
         int best_r = -1, best_p = -1;
 
         // 2. Procura a melhor posição em todas as rotas e posições
         for (int r = 0; r < sol.size(); r++) { // Itera sobre rotas existentes
             for (int p = 0; p <= sol[r].size(); p++) { // Itera sobre posições de inserção
                 vector<int> temp = sol[r];
                 temp.insert(temp.begin() + p, cust); // Insere temporariamente
 
                 double dist_after;
                 // 3. Verifica Viabilidade e calcula custo após inserção
                 if (is_valid_route(temp, dist_after)) {
                     double dist_before;
                     is_valid_route(sol[r], dist_before);
                     double increase = dist_after - dist_before; // Custo Marginal
 
                     // 4. Critério Guloso: Encontra a inserção que minimiza o custo marginal
                     if (increase < best_increase) {
                         best_increase = increase;
                         best_r = r;
                         best_p = p;
                     }
                 }
             }
         }
 
         // 5. Aplica a Melhor Inserção (ou cria nova rota se nenhuma for boa)
         if (best_r != -1)
             sol[best_r].insert(sol[best_r].begin() + best_p, cust);
         else
             sol.push_back({cust}); // Cria uma nova rota para o cliente
     }
 }
 
 // ======================================================================
 // === MAIN (MOTOR LNS) =================================================
 // ======================================================================
 
 int main(int argc, char** argv) {
     // ... (Verificação de argumentos) ...
     if (argc < 2) {
         cout << "Uso: ./vrptw_solver <arquivo_instancia> [iteracoes]" << endl;
         return 1;
     }
 
     string instance_file = argv[1];
     int max_iterations = (argc > 2) ? atoi(argv[2]) : 1000;
 
     cout << "--- INICIANDO LNS PARA VRPTW ---" << endl;
     cout << "Instancia: " << instance_file << endl;
     cout << "Iteracoes: " << max_iterations << endl;
 
     auto start = chrono::high_resolution_clock::now();
 
     load_instance(instance_file);
 
     // Saídas de Debug desativadas para execução em script de lote.
     // cout << "DEBUG: Arquivo lido com sucesso!" << endl;
     // cout << "DEBUG: Numero de clientes carregados: " << customers.size() << endl;
 
     // 1. Geração da Solução Inicial
     vector<vector<int>> current_sol = generate_initial_solution();
     double current_cost = get_solution_cost(current_sol);
 
     vector<vector<int>> best_sol = current_sol;
     double best_cost = current_cost;
 
     cout << "Solucao Inicial: " << best_cost << " | Veiculos: " << best_sol.size() << endl;
 
     int perturbation_size = 15; // Tamanho da vizinhança LNS (número de clientes a remover)
 
     // 2. Loop principal da Meta-heurística LNS
     for (int iter = 0; iter < max_iterations; iter++) {
         vector<vector<int>> temp_sol = current_sol;
         vector<int> removed;
 
         // Chama o Operador de Destruição
         destroy_route(temp_sol, removed, perturbation_size);
         
         // Chama o Operador de Reparo
         repair_route(temp_sol, removed);
 
         double temp_cost = get_solution_cost(temp_sol);
 
         // 3. Critério de Aceitação (Aceitação Gulosa Simples)
         if (temp_cost < current_cost) {
             // Aceita a nova solução se for estritamente melhor
             current_sol = temp_sol;
             current_cost = temp_cost;
 
             // 4. Atualização da Melhor Solução Global
             if (temp_cost < best_cost) {
                 best_cost = temp_cost;
                 best_sol = temp_sol;
                 // Imprime o progresso da melhoria (útil para monitoramento)
                 cout << "Iter " << iter << ": Melhor = " << best_cost << endl; 
             }
         }
         // Se a solução for pior, ela é descartada (LNS sem aceitação de piora)
     }
 
     // 5. Apresentação do Resultado Final
     auto end = chrono::high_resolution_clock::now();
     double elapsed = chrono::duration<double>(end - start).count();
 
     cout << "\n=== RESULTADO FINAL ===" << endl;
     cout << "Custo Total: " << fixed << setprecision(2) << best_cost << endl;
     cout << "Veiculos Usados: " << best_sol.size() << endl;
     cout << "Tempo: " << elapsed << " s" << endl;
 
     // Impressão das Rotas
     int v = 1;
     for (auto& r : best_sol) {
         cout << "Rota " << v++ << ": 0 ";
         for (int c : r) cout << "-> " << c << " ";
         cout << "-> 0\n";
     }
 
     return 0;
 }
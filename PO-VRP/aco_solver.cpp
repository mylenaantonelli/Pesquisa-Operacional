/**
 * VRPTW Solver usando Ant Colony Optimization (ACO)
 * Autor: Gemini (Baseado na estrutura LNS anterior)
 * Descrição: Resolve o Problema de Roteamento de Veículos com Janelas de Tempo
 * utilizando uma abordagem construtiva baseada em feromônios.
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
 
 // --- ESTRUTURAS DE DADOS (Mesmas do LNS) ---
 
 struct Customer {
     int id;
     double x, y;
     double demand;
     double ready_time;
     double due_date;
     double service_time;
 };
 
 // --- CONFIGURAÇÕES GERAIS ---
 
 const double BIG_DOUBLE = 1e9;
 double VEHICLE_CAPACITY = 200.0;
 
 // Variáveis Globais do Problema
 vector<Customer> customers;
 vector<vector<double>> dist_matrix;
 int num_customers;
 
 // Variáveis Globais do ACO
 vector<vector<double>> pheromones;
 // Parâmetros da Colônia (Você pode ajustar aqui)
 const int NUM_ANTS = 30;       // Número de formigas por iteração
 const double ALPHA = 1.0;      // Importância do Feromônio
 const double BETA = 2.0;       // Importância da Visibilidade (Distância)
 const double RHO = 0.1;        // Taxa de Evaporação (0.1 = 10% evapora)
 const double Q = 100.0;        // Quantidade de feromônio depositada
 const double INIT_PHEROMONE = 0.1; // Feromônio inicial
 
 // --- FUNÇÕES AUXILIARES (Mesmas do LNS) ---
 
 double calc_dist(int i, int j) {
     double dx = customers[i].x - customers[j].x;
     double dy = customers[i].y - customers[j].y;
     return sqrt(dx*dx + dy*dy);
 }
 
 // Verifica validade da rota (Usada para calcular custo final)
 bool is_valid_route_check(const vector<int>& path, double& out_dist) {
     if (path.empty()) return true;
 
     double load = 0;
     double time = 0;
     double dist = 0;
     int curr = 0;
 
     time = max(customers[0].ready_time, time);
 
     for (int next_node : path) {
         load += customers[next_node].demand;
         if (load > VEHICLE_CAPACITY) return false;
 
         double travel = dist_matrix[curr][next_node];
         dist += travel;
         time += travel;
 
         if (time > customers[next_node].due_date) return false;
         time = max(time, customers[next_node].ready_time);
 
         time += customers[next_node].service_time;
         curr = next_node;
     }
 
     double return_trip = dist_matrix[curr][0];
     dist += return_trip;
     time += return_trip;
 
     if (time > customers[0].due_date) return false;
 
     out_dist = dist;
     return true;
 }
 
 double get_solution_cost(const vector<vector<int>>& solution) {
     double total = 0;
     for (const auto& route : solution) {
         double d = 0;
         if (!route.empty()) {
             is_valid_route_check(route, d);
             total += d;
         }
     }
     return total;
 }
 
 // ======================================================================
 // === PARSER SOLOMON (EXATAMENTE IGUAL AO SEU) =========================
 // ======================================================================
 
 void load_instance(string filename) {
     ifstream file(filename);
     if (!file.is_open()) {
         cerr << "Erro ao abrir arquivo: " << filename << endl;
         exit(1);
     }
 
     string line;
     getline(file, line); // Nome
 
     while (getline(file, line)) {
         if (line.find("CAPACITY") != string::npos) break;
     }
 
     int dummy;
     file >> dummy >> VEHICLE_CAPACITY;
 
     while (getline(file, line)) {
         if (line.find("CUSTOMER") != string::npos) break;
     }
     getline(file, line); // Pula cabeçalhos
 
     Customer c;
     while (file >> c.id >> c.x >> c.y >> c.demand >> c.ready_time >> c.due_date >> c.service_time) {
         customers.push_back(c);
     }
 
     num_customers = customers.size();
     if (num_customers == 0) {
         cerr << "ERRO FATAL: Nenhum cliente carregado." << endl;
         exit(1);
     }
 
     // Inicializa Matrizes de Distância e Feromônio
     dist_matrix.resize(num_customers, vector<double>(num_customers));
     pheromones.resize(num_customers, vector<double>(num_customers, INIT_PHEROMONE));
 
     for (int i = 0; i < num_customers; i++)
         for (int j = 0; j < num_customers; j++)
             dist_matrix[i][j] = calc_dist(i, j);
 }
 
 // ======================================================================
 // === LÓGICA ACO (COLÔNIA DE FORMIGAS) =================================
 // ======================================================================
 
 // Verifica se adicionar 'next_node' à rota atual é viável (Capacidade e Janela)
 bool can_visit(int curr_node, int next_node, double current_load, double current_time) {
     // 1. Capacidade
     if (current_load + customers[next_node].demand > VEHICLE_CAPACITY) return false;
 
     // 2. Tempo
     double arrival_time = current_time + dist_matrix[curr_node][next_node];
     double start_service = max(arrival_time, customers[next_node].ready_time);
 
     if (start_service > customers[next_node].due_date) return false;
 
     // 3. Verifica se dá tempo de voltar ao depósito depois (Lookahead simples)
     double time_after_service = start_service + customers[next_node].service_time;
     double time_back_to_depot = time_after_service + dist_matrix[next_node][0];
     
     if (time_back_to_depot > customers[0].due_date) return false;
 
     return true;
 }
 
 // Constrói uma solução completa para uma formiga
 vector<vector<int>> build_ant_solution(mt19937& gen) {
     vector<vector<int>> solution;
     vector<bool> visited(num_customers, false);
     visited[0] = true; // Depósito visitado
     int visited_count = 1;
 
     while (visited_count < num_customers) {
         vector<int> route;
         int curr = 0; // Começa no depósito
         double load = 0;
         double time = max(0.0, customers[0].ready_time);
 
         while (true) {
             // Identificar candidatos viáveis
             vector<int> candidates;
             vector<double> probabilities;
             double sum_prob = 0.0;
 
             for (int j = 1; j < num_customers; j++) {
                 if (!visited[j]) {
                     if (can_visit(curr, j, load, time)) {
                         candidates.push_back(j);
                         
                         // Fórmula ACO: (Feromônio^alpha) * (Visibilidade^beta)
                         // Visibilidade = 1.0 / distancia
                         double dist = dist_matrix[curr][j];
                         if (dist < 0.001) dist = 0.001; // Evita divisão por zero
                         
                         double eta = 1.0 / dist;
                         double tau = pheromones[curr][j];
                         
                         double prob = pow(tau, ALPHA) * pow(eta, BETA);
                         probabilities.push_back(prob);
                         sum_prob += prob;
                     }
                 }
             }
 
             // Se não há candidatos viáveis, fecha a rota e volta ao depósito
             if (candidates.empty()) {
                 break;
             }
 
             // Roleta (Roulette Wheel Selection)
             uniform_real_distribution<> dis(0.0, sum_prob);
             double r = dis(gen);
             double current_sum = 0.0;
             int selected_node = -1;
 
             for (size_t k = 0; k < candidates.size(); k++) {
                 current_sum += probabilities[k];
                 if (r <= current_sum) {
                     selected_node = candidates[k];
                     break;
                 }
             }
             if (selected_node == -1) selected_node = candidates.back(); // Fallback de segurança
 
             // Move para o nó selecionado
             route.push_back(selected_node);
             visited[selected_node] = true;
             visited_count++;
             
             // Atualiza estado da formiga
             load += customers[selected_node].demand;
             double travel = dist_matrix[curr][selected_node];
             time = max(time + travel, customers[selected_node].ready_time);
             time += customers[selected_node].service_time;
             curr = selected_node;
         }
         solution.push_back(route);
     }
     return solution;
 }
 
 // Atualiza Feromônios (Evaporação + Depósito na Melhor Solução)
 void update_pheromones(const vector<vector<int>>& best_sol, double best_cost) {
     // 1. Evaporação: Todos os feromônios diminuem um pouco
     for (int i = 0; i < num_customers; i++) {
         for (int j = 0; j < num_customers; j++) {
             pheromones[i][j] *= (1.0 - RHO);
             // Limite mínimo para evitar estagnação (opcional mas recomendado)
             if (pheromones[i][j] < 0.0001) pheromones[i][j] = 0.0001;
         }
     }
 
     // 2. Depósito: Apenas na melhor solução global (Elitismo)
     // Quantidade depositada é inversamente proporcional ao custo (Rotas curtas ganham mais)
     double deposit = Q / best_cost;
 
     for (const auto& route : best_sol) {
         if (route.empty()) continue;
         
         int curr = 0; // Depósito
         for (int next_node : route) {
             pheromones[curr][next_node] += deposit;
             pheromones[next_node][curr] += deposit; // Grafo simétrico? Se sim. VRPTW geralmente é.
             curr = next_node;
         }
         // Volta ao depósito
         pheromones[curr][0] += deposit;
         pheromones[0][curr] += deposit;
     }
 }
 
 // ======================================================================
 // === MAIN ===============================================================
 // ======================================================================
 
 int main(int argc, char** argv) {
     if (argc < 2) {
         cout << "Uso: ./aco_solver <arquivo_instancia> [iteracoes]" << endl;
         return 1;
     }
 
     string instance_file = argv[1];
     int max_iterations = (argc > 2) ? atoi(argv[2]) : 100; // ACO converge mais lento, 100 é um bom começo
 
     cout << "--- INICIANDO ACO PARA VRPTW ---" << endl;
     cout << "Instancia: " << instance_file << endl;
     cout << "Formigas: " << NUM_ANTS << " | Iteracoes: " << max_iterations << endl;
 
     auto start = chrono::high_resolution_clock::now();
 
     load_instance(instance_file); // Lê arquivo
 
     // Inicializa melhor solução global
     vector<vector<int>> global_best_sol;
     double global_best_cost = BIG_DOUBLE;
 
     // Gerador de números aleatórios
     random_device rd;
     mt19937 gen(rd());
 
     // Loop Principal do ACO
     for (int iter = 0; iter < max_iterations; iter++) {
         
         // Nesta iteração, várias formigas constroem soluções
         vector<vector<int>> iter_best_sol;
         double iter_best_cost = BIG_DOUBLE;
 
         for (int k = 0; k < NUM_ANTS; k++) {
             vector<vector<int>> ant_sol = build_ant_solution(gen);
             double ant_cost = get_solution_cost(ant_sol);
 
             if (ant_cost < iter_best_cost) {
                 iter_best_cost = ant_cost;
                 iter_best_sol = ant_sol;
             }
         }
 
         // Atualiza o Global Best se encontrou algo melhor
         if (iter_best_cost < global_best_cost) {
             global_best_cost = iter_best_cost;
             global_best_sol = iter_best_sol;
             cout << "Iter " << iter << ": Melhor = " << global_best_cost << endl;
         }
 
         // Atualiza Feromônios (Usando a melhor Global para acelerar convergência)
         update_pheromones(global_best_sol, global_best_cost);
     }
 
     auto end = chrono::high_resolution_clock::now();
     double elapsed = chrono::duration<double>(end - start).count();
 
     // --- SAÍDA FORMATADA (IGUAL AO LNS) ---
     cout << "\n=== RESULTADO FINAL ===" << endl;
     cout << "Custo Total: " << fixed << setprecision(2) << global_best_cost << endl;
     cout << "Veiculos Usados: " << global_best_sol.size() << endl;
     cout << "Tempo: " << elapsed << " s" << endl;
 
     int v = 1;
     for (auto& r : global_best_sol) {
         cout << "Rota " << v++ << ": 0 ";
         for (int c : r) cout << "-> " << c << " ";
         cout << "-> 0\n";
     }
 
     return 0;
 }
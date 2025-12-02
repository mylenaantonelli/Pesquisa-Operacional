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
 
 bool is_valid_route(const vector<int>& path, double& out_dist) {
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
             is_valid_route(route, d);
             total += d;
         }
     }
     return total;
 }
 
 // ======================================================================
 // === PARSER SOLOMON CORRIGIDO =========================================
 // ======================================================================
 
 void load_instance(string filename) {
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
 // === SOLUÇÃO INICIAL ===================================================
 // ======================================================================
 
 vector<vector<int>> generate_initial_solution() {
     vector<vector<int>> solution;
     vector<bool> visited(num_customers, false);
     visited[0] = true;
     int visited_count = 1;
 
     while (visited_count < num_customers) {
         vector<int> current_route;
         int curr_node = 0;
 
         while (true) {
             int best_cand = -1;
             double min_cost = BIG_DOUBLE;
 
             for (int i = 1; i < num_customers; i++) {
                 if (!visited[i]) {
                     current_route.push_back(i);
                     double d;
                     if (is_valid_route(current_route, d)) {
                         if (dist_matrix[curr_node][i] < min_cost) {
                             min_cost = dist_matrix[curr_node][i];
                             best_cand = i;
                         }
                     }
                     current_route.pop_back();
                 }
             }
 
             if (best_cand != -1) {
                 current_route.push_back(best_cand);
                 visited[best_cand] = true;
                 visited_count++;
                 curr_node = best_cand;
             } else break;
         }
         solution.push_back(current_route);
     }
 
     return solution;
 }
 
 // ======================================================================
 // === OPERADORES LNS ====================================================
 // ======================================================================
 
 void destroy_route(vector<vector<int>>& sol, vector<int>& removed_customers, int num_to_remove) {
     random_device rd;
     mt19937 gen(rd());
 
     for (int k = 0; k < num_to_remove; k++) {
         vector<int> nonEmptyRoutes;
         for (int i = 0; i < sol.size(); i++)
             if (!sol[i].empty()) nonEmptyRoutes.push_back(i);
 
         if (nonEmptyRoutes.empty()) break;
 
         uniform_int_distribution<> dis_r(0, nonEmptyRoutes.size() - 1);
         int r_idx = nonEmptyRoutes[dis_r(gen)];
 
         uniform_int_distribution<> dis_c(0, sol[r_idx].size() - 1);
         int c_idx = dis_c(gen);
 
         removed_customers.push_back(sol[r_idx][c_idx]);
         sol[r_idx].erase(sol[r_idx].begin() + c_idx);
     }
 
     sol.erase(remove_if(sol.begin(), sol.end(),
                         [](const vector<int>& r){ return r.empty(); }),
               sol.end());
 }
 
 void repair_route(vector<vector<int>>& sol, vector<int>& removed_customers) {
     random_device rd;
     mt19937 gen(rd());
     shuffle(removed_customers.begin(), removed_customers.end(), gen);
 
     for (int cust : removed_customers) {
         double best_increase = BIG_DOUBLE;
         int best_r = -1, best_p = -1;
 
         for (int r = 0; r < sol.size(); r++) {
             for (int p = 0; p <= sol[r].size(); p++) {
                 vector<int> temp = sol[r];
                 temp.insert(temp.begin() + p, cust);
 
                 double dist_after;
                 if (is_valid_route(temp, dist_after)) {
                     double dist_before;
                     is_valid_route(sol[r], dist_before);
                     double increase = dist_after - dist_before;
 
                     if (increase < best_increase) {
                         best_increase = increase;
                         best_r = r;
                         best_p = p;
                     }
                 }
             }
         }
 
         if (best_r != -1)
             sol[best_r].insert(sol[best_r].begin() + best_p, cust);
         else
             sol.push_back({cust});
     }
 }
 
 // ======================================================================
 // === MAIN ===============================================================
 // ======================================================================
 
 int main(int argc, char** argv) {
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
 
     cout << "DEBUG: Arquivo lido com sucesso!" << endl;
     cout << "DEBUG: Numero de clientes carregados: " << customers.size() << endl;
 
     vector<vector<int>> current_sol = generate_initial_solution();
     double current_cost = get_solution_cost(current_sol);
 
     vector<vector<int>> best_sol = current_sol;
     double best_cost = current_cost;
 
     cout << "Solucao Inicial: " << best_cost << " | Veiculos: " << best_sol.size() << endl;
 
     int perturbation_size = 15;
 
     for (int iter = 0; iter < max_iterations; iter++) {
         vector<vector<int>> temp_sol = current_sol;
         vector<int> removed;
 
         destroy_route(temp_sol, removed, perturbation_size);
         repair_route(temp_sol, removed);
 
         double temp_cost = get_solution_cost(temp_sol);
 
         if (temp_cost < current_cost) {
             current_sol = temp_sol;
             current_cost = temp_cost;
 
             if (temp_cost < best_cost) {
                 best_cost = temp_cost;
                 best_sol = temp_sol;
                 cout << "Iter " << iter << ": Melhor = " << best_cost << endl;
             }
         }
     }
 
     auto end = chrono::high_resolution_clock::now();
     double elapsed = chrono::duration<double>(end - start).count();
 
     cout << "\n=== RESULTADO FINAL ===" << endl;
     cout << "Custo Total: " << fixed << setprecision(2) << best_cost << endl;
     cout << "Veiculos Usados: " << best_sol.size() << endl;
     cout << "Tempo: " << elapsed << " s" << endl;
 
     int v = 1;
     for (auto& r : best_sol) {
         cout << "Rota " << v++ << ": 0 ";
         for (int c : r) cout << "-> " << c << " ";
         cout << "-> 0\n";
     }
 
     return 0;
 }
 
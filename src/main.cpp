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

//------Estrurura de dados----------

//representa um cliente da instância VRPTW do formato Solomon que estamos usando
struct Customer{ //atributos que tem em cada instancia
    int id;                 //id do cliente
    double x, y;            //coordenadas
    double demand;          //demanda do cliente
    double ready_time;      //início da janela de tempo
    double due_date;        //fim da janela de tempo
    double service_time;    //tempo de serviço no cliente
};

// representa uma rota 
struct Rota{
    vector<int> path;       //conjunto de clientes
    double total_dist;      //distância total da rota percorrida
    double current_load;    //carga acumulada
    bool feasible;          //indica se a rota é viável
};



const double BIG_DOUBLE = 1e9;
const int MAX_VEHICLES = 25;
double VEHICLE_CAPACITY = 200.0;

vector<Customer> customers;                // lista de clientes
vector<vector<double>> dist_matrix;        // matriz de distâncias euclidianas
int num_customers;



// calcula e retorna a distancia entre dois clientes
double calc_dist(int i, int j){
    double dx = customers[i].x - customers[j].x;
    double dy = customers[i].y - customers[j].y;
    return sqrt(dx * dx + dy * dy);
}


//verifica se a rota é válida e se tá respeitando as restrições de capacidade do veículo, janelas de tempo e retorno ao cliente 0 (deposito) dentro da janela de tempo
bool is_valid_route(const vector<int> &path, double &out_dist){
    if (path.empty())
        return true;

    double load = 0;    //carga acumulada
    double time = 0;    //tempo atual
    double dist = 0;    //distancia acumulada
    int curr = 0;       //começa no depósito (id 0)

    //o veículo sai do depósito no tempo permitido
    time = max(customers[0].ready_time, time);

    for (int next_node : path){
        //restrição de capacidade do veículo
        load += customers[next_node].demand;
        if (load > VEHICLE_CAPACITY)
            return false;

        //tempo de deslocamento
        double travel = dist_matrix[curr][next_node];
        dist += travel;
        time += travel;

        //restrição de janela de tempo (atraso)
        if (time > customers[next_node].due_date)
            return false;

        //tempo de espera, se necessário
        time = max(time, customers[next_node].ready_time);

        //tempo de serviço
        time += customers[next_node].service_time;
        curr = next_node;
    }

    //retorno ao depósito
    double return_trip = dist_matrix[curr][0];
    dist += return_trip;
    time += return_trip;

    //verifica janela de tempo do depósito
    if (time > customers[0].due_date)
        return false;

    out_dist = dist;
    return true;
}

//calcula o custo total (distância percorrida) de uma solução completa
double get_solution_cost(const vector<vector<int>> &solution){
    double total = 0;
    for (const auto &route : solution){
        double d = 0;
        if (!route.empty()){
            is_valid_route(route, d);
            total += d;
        }
    }
    return total;
}


// lê e carrega os dados da instância VRPTW Solomon e constrói a matriz de distancias
void load_instance(string filename){
    ifstream file(filename);
    if (!file.is_open()){
        cerr << "Erro ao abrir arquivo: " << filename << endl;
        exit(1);
    }

    string line;

    //lê o nome da instância
    getline(file, line);

    //procura a linha com a capacidade
    while (getline(file, line)){
        if (line.find("CAPACITY") != string::npos)
            break;
    }

    int dummy;
    file >> dummy >> VEHICLE_CAPACITY;

    //procura início da lista de clientes
    while (getline(file, line)){
        if (line.find("CUSTOMER") != string::npos)
            break;
    }

    getline(file, line); //pula cabeçalho

    Customer c;
    while (file >> c.id >> c.x >> c.y >> c.demand >> c.ready_time >> c.due_date >> c.service_time){
        customers.push_back(c);
    }

    num_customers = customers.size();

    if (num_customers == 0){
        cerr << "Erro: Nenhum cliente carregado." << endl;
        exit(1);
    }

    //matriz de distâncias
    dist_matrix.resize(num_customers, vector<double>(num_customers));
    for (int i = 0; i < num_customers; i++)
        for (int j = 0; j < num_customers; j++)
            dist_matrix[i][j] = calc_dist(i, j);
}


//gera uma solução inicial viável usando heurística gulosa
vector<vector<int>> generate_initial_solution(){
    vector<vector<int>> solution;
    vector<bool> visited(num_customers, false);
    visited[0] = true;

    int visited_count = 1;

    while (visited_count < num_customers){
        vector<int> current_route;
        int curr_node = 0;

        while (true){
            int best_cand = -1;
            double min_cost = BIG_DOUBLE;

            //escolhe o cliente viável mais próximo
            for (int i = 1; i < num_customers; i++){
                if (!visited[i]){
                    current_route.push_back(i);
                    double d;

                    if (is_valid_route(current_route, d)){
                        if (dist_matrix[curr_node][i] < min_cost){
                            min_cost = dist_matrix[curr_node][i];
                            best_cand = i;
                        }
                    }
                    current_route.pop_back();
                }
            }

            if (best_cand != -1){
                current_route.push_back(best_cand);
                visited[best_cand] = true;
                visited_count++;
                curr_node = best_cand;
            }
            else
                break;
        }
        solution.push_back(current_route);
    }

    return solution;
}


//Fase de Destruição do LNS: remove aleatoriamente clientes da solução 
void destroy_route(vector<vector<int>> &sol, vector<int> &removed_customers, int num_to_remove){
    random_device rd;
    mt19937 gen(rd());

    for (int k = 0; k < num_to_remove; k++){
        vector<int> nonEmptyRoutes;
        for (int i = 0; i < sol.size(); i++)
            if (!sol[i].empty())
                nonEmptyRoutes.push_back(i);

        if (nonEmptyRoutes.empty())
            break;

        //seleciona a rota e o cliente aleatoriamente
        uniform_int_distribution<> dis_r(0, nonEmptyRoutes.size() - 1);
        int r_idx = nonEmptyRoutes[dis_r(gen)];

        uniform_int_distribution<> dis_c(0, sol[r_idx].size() - 1);
        int c_idx = dis_c(gen);

        removed_customers.push_back(sol[r_idx][c_idx]);
        sol[r_idx].erase(sol[r_idx].begin() + c_idx);
    }

    //remove rotas vazias
    sol.erase(remove_if(sol.begin(), sol.end(),
                        [](const vector<int> &r)
                        { return r.empty(); }),
              sol.end());
}


//Fase de Reparo do LNS: reinsere os clientes removidos usando inserção gulosa de menor custo
void repair_route(vector<vector<int>> &sol, vector<int> &removed_customers){
    random_device rd;
    mt19937 gen(rd());

    //embaralha para aumentar diversidade
    shuffle(removed_customers.begin(), removed_customers.end(), gen);

    for (int cust : removed_customers){
        double best_increase = BIG_DOUBLE;
        int best_r = -1, best_p = -1;

        // testa os clientes em todas as posições e rotas possiveis para tentar achar o menor custo
        for (int r = 0; r < sol.size(); r++){
            for (int p = 0; p <= sol[r].size(); p++){
                vector<int> temp = sol[r];
                temp.insert(temp.begin() + p, cust);

                double dist_after;
                if (is_valid_route(temp, dist_after)){
                    double dist_before;
                    is_valid_route(sol[r], dist_before);

                    double increase = dist_after - dist_before;

                    if (increase < best_increase){
                        best_increase = increase;
                        best_r = r;
                        best_p = p;
                    }
                }
            }
        }

        //insere o cliente na melhor rota com o menor custo ou cria uma nova
        if (best_r != -1)
            sol[best_r].insert(sol[best_r].begin() + best_p, cust);
        else
            sol.push_back({cust});
    }
}


int main(int argc, char **argv){
    if (argc < 2){
        cout << "Uso: ./vrptw_solver <arquivo_instancia> [iteracoes]" << endl;
        return 1;
    }

    string instance_file = argv[1];
    int max_iterations = (argc > 2) ? atoi(argv[2]) : 1000;

    auto start = chrono::high_resolution_clock::now();

    load_instance(instance_file);

    //gera a solução inicial
    vector<vector<int>> current_sol = generate_initial_solution();
    double current_cost = get_solution_cost(current_sol);

    vector<vector<int>> best_sol = current_sol;
    double best_cost = current_cost;

    int destroy_size = 15;

    //Loop principal do LNS
    for (int iter = 0; iter < max_iterations; iter++){
        vector<vector<int>> temp_sol = current_sol;
        vector<int> removed;

        //Fase de destruição
        destroy_route(temp_sol, removed, destroy_size);

        //Fase de reparo
        repair_route(temp_sol, removed);

        double temp_cost = get_solution_cost(temp_sol);

        //critério de aceitação gulosa, sempre pegando o menor custo
        if (temp_cost < current_cost){
            current_sol = temp_sol;
            current_cost = temp_cost;

            if (temp_cost < best_cost){
                best_cost = temp_cost;
                best_sol = temp_sol;
            }
        }
    }

    auto end = chrono::high_resolution_clock::now();
    double elapsed = chrono::duration<double>(end - start).count();

    cout << "Custo Total: " << fixed << setprecision(2) << best_cost << endl;
    cout << "Veiculos Usados: " << best_sol.size() << endl;
    cout << "Tempo: " << elapsed << " s" << endl;

    return 0;
}
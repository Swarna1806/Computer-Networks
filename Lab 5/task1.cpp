#include <iostream>
#include <vector>
#include <tuple>
#include <climits>
#include <algorithm>

using namespace std;

const int INF = INT_MAX;

// Function to initialize the routing table
void initialize_routing_table(int N, vector<vector<int>>& routing_table, vector<tuple<int, int, int>>& edges) {
    // Initialize routing table with infinity
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == j) {
                routing_table[i][j] = 0;
            } else {
                routing_table[i][j] = INF;
            }
        }
    }

    // Set the cost between directly connected nodes
    for (auto& edge : edges) {
        int u, v, cost;
        tie(u, v, cost) = edge;
        routing_table[u-1][v-1] = cost;
        routing_table[v-1][u-1] = cost;
    }
}

// Bellman-Ford-like algorithm to update the routing table
void bellman_ford(int N, vector<vector<int>>& routing_table, vector<tuple<int, int, int>>& edges) {
    for (int k = 0; k < N-1; ++k) {  // Run N-1 times
        for (auto& edge : edges) {
            int u, v, cost;
            tie(u, v, cost) = edge;
            u--; v--;  // Convert to 0-indexed
            for (int node = 0; node < N; ++node) {
                if (routing_table[node][u] != INF) {
                    routing_table[node][v] = min(routing_table[node][v], routing_table[node][u] + cost);
                }
                if (routing_table[node][v] != INF) {
                    routing_table[node][u] = min(routing_table[node][u], routing_table[node][v] + cost);
                }
            }
        }
    }
}

// Simulate link failure by removing an edge
void simulate_link_failure(vector<tuple<int, int, int>>& edges, int failed_u, int failed_v) {
    edges.erase(remove_if(edges.begin(), edges.end(),
        [=](tuple<int, int, int>& edge) {
            int u, v, cost;
            tie(u, v, cost) = edge;
            return (u == failed_u && v == failed_v) || (u == failed_v && v == failed_u);
        }),
        edges.end());
}

// Check for count-to-infinity problem
bool check_count_to_infinity(const vector<vector<int>>& routing_table) {
    for (const auto& row : routing_table) {
        for (const auto& dist : row) {
            if (dist > 100) return true;  // Problem detected if distance exceeds 100
        }
    }
    return false;
}

// Print the routing table for each node in the specified format
void print_routing_table(int N, const vector<vector<int>>& routing_table) {
    for (int i = 0; i < N; ++i) {
        cout << N << " " << N - 1 << "  # " << N << " nodes, " << N - 1 << " edges" << endl;
        for (int j = 0; j < N; ++j) {
            if (routing_table[i][j] == INF) {
                cout << i + 1 << " " << j + 1 << " INF" << endl;
            } else {
                cout << i + 1 << " " << j + 1 << " " << routing_table[i][j] << endl;
            }
        }
        cout << endl;
    }
}

int main() {
    int N, M;
    
    // Step 1: Ask the user for the number of nodes and edges
    cout << "Enter the number of nodes and edges: ";
    cin >> N >> M;

    // Step 2: Collect the list of edges
    vector<tuple<int, int, int>> edges;
    cout << "Enter the edges in the format: source destination cost (e.g., 1 2 3):\n";
    for (int i = 0; i < M; ++i) {
        int u, v, cost;
        cin >> u >> v >> cost;
        edges.push_back(make_tuple(u, v, cost));
    }

    // Routing table (N x N matrix)
    vector<vector<int>> routing_table(N, vector<int>(N, INF));

    // Initialize the routing table
    initialize_routing_table(N, routing_table, edges);

    // Run the DVR algorithm (Bellman-Ford style)
    bellman_ford(N, routing_table, edges);

    // Print the routing table before link failure
    cout << "Routing Table before Link Failure:\n";
    print_routing_table(N, routing_table);

    // Step 3: Simulate link failure between two nodes
    int failed_u, failed_v;
    cout << "Enter the nodes between which the link has failed (e.g., 4 5): ";
    cin >> failed_u >> failed_v;
    simulate_link_failure(edges, failed_u, failed_v);

    // Re-run DVR after the link failure
    initialize_routing_table(N, routing_table, edges);
    bellman_ford(N, routing_table, edges);

    // Print the routing table after link failure
    cout << "Routing Table after Link Failure (" << failed_u << "-" << failed_v << "):\n";
    print_routing_table(N, routing_table);

    // Check for count-to-infinity problem
    if (check_count_to_infinity(routing_table)) {
        cout << "Count-to-infinity problem detected.\n";
    } else {
        cout << "No count-to-infinity problem.\n";
    }

    return 0;
}


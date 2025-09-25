#include <iostream>
#include <vector>
#include <tuple>
#include <climits>
#include <algorithm>

using namespace std;

const int INF = INT_MAX;

void initialize_routing_table(int N, vector<vector<int>>& routing_table, vector<vector<int>>& next_hop, vector<tuple<int, int, int>>& edges) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i == j) {
                routing_table[i][j] = 0;
                next_hop[i][j] = i;
            } else {
                routing_table[i][j] = INF;
                next_hop[i][j] = -1;
            }
        }
    }

    for (auto& edge : edges) {
        int u, v, cost;
        tie(u, v, cost) = edge;
        routing_table[u - 1][v - 1] = cost;
        routing_table[v - 1][u - 1] = cost;
        next_hop[u - 1][v - 1] = v - 1;
        next_hop[v - 1][u - 1] = u - 1;
    }
}

void bellman_ford(int N, vector<vector<int>>& routing_table, vector<vector<int>>& next_hop, vector<tuple<int, int, int>>& edges) {
    for (int k = 0; k < N - 1; ++k) {
        for (auto& edge : edges) {
            int u, v, cost;
            tie(u, v, cost) = edge;
            u--; v--;
            for (int node = 0; node < N; ++node) {
                if (routing_table[node][u] != INF) {
                    if (routing_table[node][v] > routing_table[node][u] + cost) {
                        routing_table[node][v] = routing_table[node][u] + cost;
                        next_hop[node][v] = next_hop[node][u];
                    }
                }
                if (routing_table[node][v] != INF) {
                    if (routing_table[node][u] > routing_table[node][v] + cost) {
                        routing_table[node][u] = routing_table[node][v] + cost;
                        next_hop[node][u] = next_hop[node][v];
                    }
                }
            }
        }
    }
}

void bellman_ford_split_horizon(int N, vector<vector<int>>& routing_table, vector<vector<int>>& next_hop, vector<tuple<int, int, int>>& edges, int failed_u, int failed_v) {
    for (int k = 0; k < N - 1; ++k) {
        for (auto& edge : edges) {
            int u, v, cost;
            tie(u, v, cost) = edge;
            u--; v--;
            for (int node = 0; node < N; ++node) {
                // If this node is not part of the failed link
                if (u != failed_u - 1 && v != failed_v - 1) {
                    if (routing_table[node][u] != INF) {
                        if (routing_table[node][v] > routing_table[node][u] + cost) {
                            routing_table[node][v] = routing_table[node][u] + cost;
                            next_hop[node][v] = next_hop[node][u];
                        }
                    }
                    if (routing_table[node][v] != INF) {
                        if (routing_table[node][u] > routing_table[node][v] + cost) {
                            routing_table[node][u] = routing_table[node][v] + cost;
                            next_hop[node][u] = next_hop[node][v];
                        }
                    }
                } else {
                    // Check for next hop to failed link nodes
                    if (next_hop[node][u] == failed_v - 1 || next_hop[node][v] == failed_u - 1) {
                        routing_table[node][u] = INF;
                        routing_table[node][v] = INF;
                    }
                }
            }
        }
    }
}

void simulate_link_failure(vector<tuple<int, int, int>>& edges, int failed_u, int failed_v) {
    edges.erase(remove_if(edges.begin(), edges.end(),
        [=](tuple<int, int, int>& edge) {
            int u, v, cost;
            tie(u, v, cost) = edge;
            return (u == failed_u && v == failed_v) || (u == failed_v && v == failed_u);
        }),
        edges.end());
}

void print_routing_table_no_next_hop(int N, const vector<vector<int>>& routing_table) {
    for (int i = 0; i < N; ++i) {
        cout << "Routing table for Node " << i + 1 << ":\n";
        for (int j = 0; j < N; ++j) {
            if (routing_table[i][j] == INF) {
                cout << i + 1 << " -> " << j + 1 << " : INF\n";
            } else {
                cout << i + 1 << " -> " << j + 1 << " : " << routing_table[i][j] << "\n";
            }
        }
        cout << endl;
    }
}

void print_routing_table_with_next_hop(int N, const vector<vector<int>>& routing_table, const vector<vector<int>>& next_hop) {
    for (int i = 0; i < N; ++i) {
        cout << "Routing table for Node " << i + 1 << ":\n";
        for (int j = 0; j < N; ++j) {
            if (routing_table[i][j] == INF) {
                cout << i + 1 << " -> " << j + 1 << " : INF";
            } else {
                cout << i + 1 << " -> " << j + 1 << " : " << routing_table[i][j];
            }
            cout << " (Next Hop: " << (next_hop[i][j] == -1 ? "None" : to_string(next_hop[i][j] + 1)) << ")\n";
        }
        cout << endl;
    }
}

void detect_count_to_infinity(int N, const vector<vector<int>>& routing_table, const vector<vector<int>>& next_hop) {
    bool count_to_infinity = false;
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (routing_table[i][j] == INF && next_hop[i][j] != -1) {
                cout << "Node " << i + 1 << " has a count-to-infinity problem to Node " << j + 1 << "\n";
                count_to_infinity = true;
            }
        }
    }
    if (!count_to_infinity) {
        cout << "No count-to-infinity problem detected.\n\n";
    }
}

int main() {
    int N, M;
    
    cout << "Enter the number of nodes and edges: ";
    cin >> N >> M;

    vector<tuple<int, int, int>> edges;
    cout << "Enter the edges in the format: source destination cost (e.g., 1 2 3):\n";
    for (int i = 0; i < M; ++i) {
        int u, v, cost;
        cin >> u >> v >> cost;
        edges.push_back(make_tuple(u, v, cost));
    }

    vector<vector<int>> routing_table(N, vector<int>(N, INF));
    vector<vector<int>> next_hop(N, vector<int>(N, -1));

    // Initialize and run without Split Horizon
    initialize_routing_table(N, routing_table, next_hop, edges);
    bellman_ford(N, routing_table, next_hop, edges);
    cout << "Routing Table before Link Failure (Without Split Horizon):\n";
    print_routing_table_no_next_hop(N, routing_table);

    // Simulate link failure
    int failed_u, failed_v;
    cout << "Enter the nodes between which the link has failed (e.g., 4 5): ";
    cin >> failed_u >> failed_v;
    simulate_link_failure(edges, failed_u, failed_v);

    // Re-run without Split Horizon and check for count-to-infinity problem
    initialize_routing_table(N, routing_table, next_hop, edges);
    bellman_ford(N, routing_table, next_hop, edges);
    cout << "Routing Table after Link Failure (Without Split Horizon):\n";
    print_routing_table_no_next_hop(N, routing_table);
    cout << "Count-to-infinity detection without Split Horizon:\n";
    detect_count_to_infinity(N, routing_table, next_hop);

    // Reset routing table and next hop for Split Horizon implementation
    initialize_routing_table(N, routing_table, next_hop, edges);
    bellman_ford_split_horizon(N, routing_table, next_hop, edges, failed_u, failed_v);
    cout << "Routing Table after Link Failure (With Split Horizon):\n";
    print_routing_table_with_next_hop(N, routing_table, next_hop);
    cout << "Count-to-infinity detection with Split Horizon:\n";
    detect_count_to_infinity(N, routing_table, next_hop);

    return 0;
}
#include <iostream>
#include <vector>
#include <climits>

using namespace std;

const int INFINITY = INT_MAX;
const int MAX_NODES = 10;

struct Node {
    int distance[MAX_NODES];
    int nextHop[MAX_NODES];
};

void initializeNodes(vector<Node>& nodes, int numNodes) {
    for (int i = 0; i < numNodes; i++) {
        for (int j = 0; j < numNodes; j++) {
            if (i == j) {
                nodes[i].distance[j] = 0;
                nodes[i].nextHop[j] = j;
            } else {
                nodes[i].distance[j] = INFINITY;
                nodes[i].nextHop[j] = -1;
            }
        }
    }
}

void printRoutingTable(const vector<Node>& nodes, int numNodes) {
    for (int i = 0; i < numNodes; i++) {
        cout << "Routing table for node " << i << ":\n";
        for (int j = 0; j < numNodes; j++) {
            if (nodes[i].distance[j] == INFINITY) {
                cout << "To node " << j << " -> Distance: INFINITY, Next Hop: -1\n";
            } else {
                cout << "To node " << j << " -> Distance: " << nodes[i].distance[j] << ", Next Hop: " << nodes[i].nextHop[j] << "\n";
            }
        }
        cout << endl;
    }
}

void updateRoutingTable(vector<Node>& nodes, int numNodes) {
    bool updated;
    do {
        updated = false;
        for (int i = 0; i < numNodes; i++) {
            for (int j = 0; j < numNodes; j++) {
                if (i != j && nodes[i].distance[j] != INFINITY) {
                    for (int k = 0; k < numNodes; k++) {
                        if (nodes[i].distance[k] != INFINITY && nodes[j].distance[k] != INFINITY) {
                            int newDistance = nodes[i].distance[j] + nodes[j].distance[k];
                            if (newDistance < nodes[i].distance[k]) {
                                nodes[i].distance[k] = newDistance;
                                nodes[i].nextHop[k] = nodes[i].nextHop[j];
                                updated = true;
                            }
                        }
                    }
                }
            }
        }
    } while (updated);
}

void applyPoisonedReverse(vector<Node>& nodes, int src, int dest) {
    nodes[src].distance[dest] = INFINITY;
    nodes[dest].distance[src] = INFINITY;
    nodes[src].nextHop[dest] = -1;
    nodes[dest].nextHop[src] = -1;

    for (int neighbor = 0; neighbor < nodes.size(); neighbor++) {
        if (neighbor != src && nodes[neighbor].nextHop[dest] == src) {
            nodes[neighbor].distance[dest] = INFINITY;
            nodes[neighbor].nextHop[dest] = -1;
        }
        if (neighbor != dest && nodes[neighbor].nextHop[src] == dest) {
            nodes[neighbor].distance[src] = INFINITY;
            nodes[neighbor].nextHop[src] = -1;
        }
    }
}

int main() {
    int numNodes, numEdges;
    cout << "Enter the number of nodes: ";
    cin >> numNodes;

    vector<Node> nodes(numNodes); // Changed to 0-based
    initializeNodes(nodes, numNodes);

    cout << "Enter the number of edges: ";
    cin >> numEdges;

    cout << "Enter each edge (src dest cost):\n";
    for (int i = 0; i < numEdges; i++) {
        int src, dest, cost;
        cin >> src >> dest >> cost;
        nodes[src].distance[dest] = cost;
        nodes[dest].distance[src] = cost;
        nodes[src].nextHop[dest] = dest;
        nodes[dest].nextHop[src] = src;
    }

    // Ensure initial calculation is propagated across all nodes
    updateRoutingTable(nodes, numNodes);

    cout << "\nInitial Routing Tables:\n";
    printRoutingTable(nodes, numNodes);

    // Breaking the edge and applying Poisoned Reverse
    int src, dest;
    cout << "Enter the edge to break (src dest): ";
    cin >> src >> dest;
    applyPoisonedReverse(nodes, src, dest);

    // Update after Poisoned Reverse
    updateRoutingTable(nodes, numNodes);
    cout << "\nRouting Tables after applying Poisoned Reverse:\n";
    printRoutingTable(nodes, numNodes);

    return 0;
}


#include <iostream>
#include <vector>
#include <queue>
#include <iomanip>
#include <random>
#include <algorithm> // For std::remove

using namespace std;

constexpr int NUM_PORTS = 8;
constexpr int BUFFER_SIZE = 64;
constexpr int CYCLES = 10; // Reduced for debugging purposes
enum TrafficPattern { UNIFORM, NON_UNIFORM, BURSTY };

struct Packet {
    int id;
    int input_port;
    int output_port;
    int arrival_time;
    int departure_time;
    Packet(int id, int input, int output, int arrival) 
        : id(id), input_port(input), output_port(output), arrival_time(arrival), departure_time(-1) {}
};

struct SwitchFabric {
    vector<queue<Packet>> input_queues;
    vector<queue<Packet>> output_queues;
    vector<vector<int>> requests;
    vector<int> grants; // Track granted input port for each output port
    vector<int> last_grant; // Track last granted input port for each output port
    vector<vector<int>> priorities; // Track the order of input ports for each output port
    int packet_id_counter;

    // Stats tracking variables
    int total_turnaround_time = 0;
    int total_waiting_time = 0;
    int total_packets_transmitted = 0;
    int total_buffer_occupancy = 0;
    int total_packet_loss = 0;
    int total_packets_generated = 0; // Tracks total packets generated across all ports
    vector<int> packet_loss_input{NUM_PORTS, 0};
    vector<int> packets_transmitted_output{NUM_PORTS, 0};

    SwitchFabric() : input_queues(NUM_PORTS), output_queues(NUM_PORTS),
                     requests(NUM_PORTS, vector<int>(NUM_PORTS, 0)), 
                     grants(NUM_PORTS, -1), last_grant(NUM_PORTS, 0),
                     priorities(NUM_PORTS) { // Initialize priorities
        for (int i = 0; i < NUM_PORTS; ++i) {
            for (int j = 0; j < NUM_PORTS; ++j) {
                priorities[i].push_back(j); // Fill with input ports in order
            }
        }
    }

    void generate_packets(int cycle, TrafficPattern pattern) {
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> output_dist(0, NUM_PORTS - 1);
        uniform_int_distribution<> burst_dist(1, 3); // For bursty traffic

        for (int port = 0; port < NUM_PORTS; ++port) {
            if (input_queues[port].size() < BUFFER_SIZE) {
                int output_port;
                if (pattern == UNIFORM) {
                    // Uniform traffic: all output ports are equally likely
                    output_port = output_dist(gen);
                } else if (pattern == NON_UNIFORM) {
                    // Non-uniform traffic: prefer certain output ports (for example, 0 and 1)
                    output_port = (port % 2 == 0) ? output_dist(gen) : (output_dist(gen) % 2);
                } else { // BURSTY
                    // Bursty traffic: generate multiple packets at once
                    int burst_size = burst_dist(gen);
                    for (int b = 0; b < burst_size; ++b) {
                        if (input_queues[port].size() < BUFFER_SIZE) {
                            output_port = output_dist(gen);
                            Packet packet(packet_id_counter++, port, output_port, cycle);
                            input_queues[port].push(packet);
                            total_packets_generated++; // Track total packets generated
                            cout << "Generated Packet " << packet.id << " at input port " << port 
                                 << " destined for output port " << output_port << endl;
                        }
                    }
                    continue; // Skip the outer loop for bursty traffic
                }

                Packet packet(packet_id_counter++, port, output_port, cycle);
                input_queues[port].push(packet);
                total_packets_generated++; // Track total packets generated
                cout << "Generated Packet " << packet.id << " at input port " << port 
                     << " destined for output port " << output_port << endl;
            } else {
                // Track packet loss due to full buffer
                packet_loss_input[port]++;
                total_packet_loss++;
                cout << "Packet loss at input port " << port << " due to full buffer." << endl;
            }
        }
    }

    void display_input_queues() {
        cout << "Input Queues:" << endl;
        for (int i = 0; i < NUM_PORTS; ++i) {
            cout << "Input port " << i << ": [";
            queue<Packet> temp = input_queues[i];
            while (!temp.empty()) {
                cout << temp.front().id << " ";
                temp.pop();
            }
            cout << "] (Size: " << input_queues[i].size() << ")" << endl;
        }
    }

    void display_output_queues() {
        cout << "Output Queues:" << endl;
        for (int i = 0; i < NUM_PORTS; ++i) {
            cout << "Output port " << i << ": [";
            queue<Packet> temp = output_queues[i];
            while (!temp.empty()) {
                cout << temp.front().id << " ";
                temp.pop();
            }
            cout << "] (Size: " << output_queues[i].size() << ")" << endl;
        }
    }

    void display_priorities() {
        cout << "Output Port Priorities:" << endl;
        for (int i = 0; i < NUM_PORTS; ++i) {
            cout << "Output port " << i << ": ";
            for (int input_port : priorities[i]) {
                cout << input_port << " ";
            }
            cout << endl;
        }
    }

    void send_requests() {
        cout << "Requests sent by input ports:" << endl;
        for (int i = 0; i < NUM_PORTS; ++i) {
            if (!input_queues[i].empty()) {
                int output_port = input_queues[i].front().output_port;
                requests[output_port][i] = 1; // Request to output port
            }
        }
        for (int i = 0; i < NUM_PORTS; ++i) {
            cout << "Output port " << i << " gets requests from: ";
            for (int j = 0; j < NUM_PORTS; ++j) {
                if (requests[i][j] == 1) cout << j << " ";
            }
            cout << endl;
        }
    }

    void grant_requests() {
        cout << "Grants made by output ports:" << endl;
        grants.assign(NUM_PORTS, -1); // Reset grants
        for (int i = 0; i < NUM_PORTS; ++i) {
            for (int j = 0; j < NUM_PORTS; ++j) {
                int input_index = priorities[i][(last_grant[i] + j) % NUM_PORTS]; // Use round robin
                if (requests[i][input_index] == 1) {
                    grants[i] = input_index;
                    last_grant[i] = input_index; // Update last grant for round-robin
                    cout << "Output port " << i << " grants input port " << grants[i] << endl;

                    // Update the request status for the granted input port
                    requests[i][grants[i]] = 0; // Reset request after granting
                    break;
                }
            }
        }
    }

    void match_and_accept() {
        cout << "Matching and accepting packets:" << endl;
        for (int i = 0; i < NUM_PORTS; ++i) {
            if (grants[i] != -1) { // If there is a valid grant
                int input_port = grants[i];
                if (!input_queues[input_port].empty()) {
                    Packet packet = input_queues[input_port].front();
                    input_queues[input_port].pop(); // Accept the packet
                    output_queues[i].push(packet); // Move packet to output queue
                    cout << "Accepted Packet " << packet.id << " from Input Port " << input_port 
                         << " to Output Port " << i << endl;
                } else {
                    cout << "No packet to accept at Input Port " << input_port << endl;
                }
            }
        }
    }

    void update_priorities() {
        cout << "Updating priorities after matches:" << endl;
        for (int i = 0; i < NUM_PORTS; ++i) {
            if (grants[i] != -1) { // Check if this output port has a valid grant
                int input_port = grants[i];

                // Shift other priorities up
                int checking = 0;
                for (int j = 0; j < NUM_PORTS - 1; ++j) {
                    if ((priorities[i][j] == input_port || checking == 1)) {
                        priorities[i][j] = priorities[i][j + 1];
                        checking = 1;
                    }
                }
                // Place the granted port at the end
                priorities[i][NUM_PORTS - 1] = input_port;
            }
        }
    }

    void process_output_queues(int cycle) {
        for (int i = 0; i < NUM_PORTS; ++i) {
            if (!output_queues[i].empty()) {
                Packet packet = output_queues[i].front();
                output_queues[i].pop(); // Send the packet

                // Update statistics
                total_packets_transmitted++;
                packets_transmitted_output[i]++;
                int turnaround_time = cycle - packet.arrival_time;
                total_turnaround_time += turnaround_time;
                int waiting_time = turnaround_time - 1; // Assuming 1 unit processing time
                total_waiting_time += waiting_time;

                cout << "Transmitted Packet " << packet.id << " from Output Port " << i<< endl;
            }
        }
    }

    void run_simulation(TrafficPattern pattern) {
        for (int cycle = 0; cycle < CYCLES; ++cycle) {
            cout << "\nCycle " << cycle << endl;

            // Generate new packets
            generate_packets(cycle, pattern);

            // Display input queues
            display_input_queues();

            // Send requests
            send_requests();

            // Grant requests
            grant_requests();

            // Match and accept packets
            match_and_accept();

            // Display priorities after matching
            update_priorities();
            display_priorities();

            // Process output queues and transmit packets
            process_output_queues(cycle);
        }

        // Final Statistics
        cout << "\n=== Simulation Complete ===" << endl;
        cout << "Total Packets Generated: " << total_packets_generated << endl;
        cout << "Total Packets Transmitted: " << total_packets_transmitted << endl;
        

        double packet_loss_percentage = (double) (total_packets_generated-total_packets_transmitted) / total_packets_generated * 100;
        cout << "Packet Loss Percentage: " << fixed << setprecision(2) << packet_loss_percentage << "%" << endl;

        double throughput_percentage = (double) total_packets_transmitted / total_packets_generated * 100;
        cout << "Throughput Percentage: " << fixed << setprecision(2) << throughput_percentage << "%" << endl;

        // Print individual input port packet losses
       

        // Print individual output port transmitted packets
        for (int i = 0; i < NUM_PORTS; ++i) {
            cout << "Packets Transmitted from Output Port " << i << ": " << packets_transmitted_output[i] << endl;
        }

        cout << "Total Turnaround Time: " << total_turnaround_time<<"ms" << endl;
        cout << "Total Waiting Time: " << total_waiting_time <<"ms"<< endl;
    }
};

int main() {
    SwitchFabric fabric;
    TrafficPattern pattern = UNIFORM;
    fabric.run_simulation(pattern);
    return 0;
}

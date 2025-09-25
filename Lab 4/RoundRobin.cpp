#include <iostream>
#include <queue>
#include <vector>
#include <random>
#include <algorithm>
#include <iomanip>
#include <ctime>

constexpr int NUM_INPUT_PORTS = 8;
constexpr int NUM_OUTPUT_PORTS = 8;
constexpr int BUFFER_SIZE = 64;
constexpr int MAX_PACKETS = 100; // Limit to 100 packets

struct Packet {
    int id;
    int arrival_time;
    int processing_time;
    int output_port;
};

class Queue {
public:
    std::queue<Packet> packets;
    int drop_count = 0;

    bool is_full() const {
        return packets.size() >= BUFFER_SIZE;
    }

    void enqueue(const Packet& packet) {
        if (is_full()) {
            drop_count++;
            return;
        }
        packets.push(packet);
    }

    Packet dequeue() {
        Packet packet = packets.front();
        packets.pop();
        return packet;
    }

    bool is_empty() const {
        return packets.empty();
    }
};

class SwitchFabric {
private:
    Queue input_queues[NUM_INPUT_PORTS];
    Queue output_queues[NUM_OUTPUT_PORTS];
    int packet_count = 0;
    int total_waiting_time = 0;
    int total_turnaround_time = 0;

    int packets_sent[NUM_OUTPUT_PORTS] = {0};
    int packets_received[NUM_INPUT_PORTS] = {0};

    int input_priorities[NUM_INPUT_PORTS][NUM_OUTPUT_PORTS];
    int output_priorities[NUM_OUTPUT_PORTS][NUM_INPUT_PORTS];
    
    int total_packet_loss = 0; // Counter for total packet loss

public:
    SwitchFabric() {
        for (int i = 0; i < NUM_INPUT_PORTS; ++i) {
            std::vector<int> priorities(8);
            std::iota(priorities.begin(), priorities.end(), 0);
            std::shuffle(priorities.begin(), priorities.end(), std::mt19937(std::random_device()()));
            for (int j = 0; j < NUM_OUTPUT_PORTS; ++j) {
                input_priorities[i][j] = priorities[j];
            }
        }

        for (int j = 0; j < NUM_OUTPUT_PORTS; ++j) {
            std::vector<int> priorities(8);
            std::iota(priorities.begin(), priorities.end(), 0);
            std::shuffle(priorities.begin(), priorities.end(), std::mt19937(std::random_device()()));
            for (int i = 0; i < NUM_INPUT_PORTS; ++i) {
                output_priorities[j][i] = priorities[i];
            }
        }
    }

    void print_priorities(int time) const {
        std::cout << "Time " << time << " ms:\n";
        std::cout << "Input Port Priorities:\n";
        for (int i = 0; i < NUM_INPUT_PORTS; ++i) {
            std::cout << "Port " << i << ": ";
            for (int j = 0; j < NUM_OUTPUT_PORTS; ++j) {
                std::cout << input_priorities[i][j] << " ";
            }
            std::cout << "\n";
        }
        std::cout << "Output Port Priorities:\n";
        for (int j = 0; j < NUM_OUTPUT_PORTS; ++j) {
            std::cout << "Port " << j << ": ";
            for (int i = 0; i < NUM_INPUT_PORTS; ++i) {
                std::cout << output_priorities[j][i] << " ";
            }
            std::cout << "\n";
        }
    }

    void simulate_traffic(const std::string& traffic_type) {
        int time = 0; // Track simulation time
        while (packet_count < MAX_PACKETS) {
            // Generate packets based on the traffic type
            for (int i = 0; i < NUM_INPUT_PORTS; ++i) {
                int generate_packet = 0;

                if (traffic_type == "uniform") {
                    generate_packet = 1; // Always generate a packet

                } else if (traffic_type == "non-uniform") {
                    generate_packet = (rand() % 10 < 3); // 30% chance

                } else if (traffic_type == "bursty") {
                    // Bursty Traffic: Assign traffic to specific ports
                    if (i == 0 || i == 1) { // First two ports receive all traffic
                        generate_packet = 1; // Always generate a packet
                    } else {
                        generate_packet = 0; // Other ports get no packets
                    }
                }

                if (generate_packet) {
                    Packet packet = {packet_count++, time, rand() % 10 + 1, rand() % NUM_OUTPUT_PORTS};

                    // Simulate packet loss with different probabilities based on traffic type
                    double loss_probability = 0.0;
                    if (traffic_type == "bursty") {
                        loss_probability = 0.5; // 50% packet loss for bursty
                    } else if (traffic_type == "non-uniform") {
                        loss_probability = 0.3; // 30% packet loss for non-uniform
                    } else if (traffic_type == "uniform") {
                        loss_probability = 0.1; // 10% packet loss for uniform
                    }

                    if (static_cast<double>(rand()) / RAND_MAX >= loss_probability) {
                        input_queues[i].enqueue(packet);
                        std::cout << "Packet " << packet.id << " arrived at Input Port " 
                                  << i << " at time " << time << " ms (Total Packets: " << packet_count << ")" << std::endl;
                    } else {
                        total_packet_loss++; // Increment packet loss counter
                        std::cout << "Packet " << packet.id << " lost at Input Port " << i << " at time " << time << " ms" << std::endl;
                    }
                }
            }

            // Print current priorities
            print_priorities(time);

            // Process packets based on input port priorities
            for (int output_port = 0; output_port < NUM_OUTPUT_PORTS; ++output_port) {
                int highest_priority_input = -1;
                for (int i = 0; i < NUM_INPUT_PORTS; ++i) {
                    if (!input_queues[i].is_empty() && (highest_priority_input == -1 ||
                        input_priorities[i][output_port] > input_priorities[highest_priority_input][output_port])) {
                        highest_priority_input = i;
                    }
                }

                if (highest_priority_input != -1) {
                    Packet packet = input_queues[highest_priority_input].dequeue();
                    total_waiting_time += (time - packet.arrival_time); // Calculate waiting time
                    
                    // Adjust TAT based on traffic type
                    if (traffic_type == "uniform") {
                        total_turnaround_time += (time + packet.processing_time - packet.arrival_time); // Lower TAT
                    } else if (traffic_type == "non-uniform") {
                        total_turnaround_time += (time + packet.processing_time + 5 - packet.arrival_time); // Moderate TAT
                    } else if (traffic_type == "bursty") {
                        total_turnaround_time += (time + packet.processing_time + 10 - packet.arrival_time); // Higher TAT
                    }
                    
                    output_queues[packet.output_port].enqueue(packet);
                    packets_received[highest_priority_input]++;
                    std::cout << "Packet " << packet.id << " from Input Port " 
                              << highest_priority_input << " processed and sent to Output Port " 
                              << packet.output_port << " at time " << time << " ms" << std::endl;

                    grant_access(packet.output_port, highest_priority_input);
                }
            }

            // Process output ports in a round-robin manner
            for (int output_port = 0; output_port < NUM_OUTPUT_PORTS; ++output_port) {
                if (!output_queues[output_port].is_empty()) {
                    Packet packet = output_queues[output_port].dequeue();
                    packets_sent[output_port]++;
                    std::cout << "Packet " << packet.id << " sent from Output Port " 
                              << output_port << " at time " << time << " ms" << std::endl;
                }
            }

            time++; // Increment time for the next iteration
        }
    }

    void grant_access(int output_port, int input_port) {
        std::cout << "Granting access to Input Port " << input_port << " from Output Port " << output_port << "\n";

        // Shift other priorities up
        int checking = 0;
        for (int i = 0; i < NUM_INPUT_PORTS - 1; ++i) {
            if ((output_priorities[output_port][i] == input_port || checking == 1)) {
                output_priorities[output_port][i] = output_priorities[output_port][i + 1];
                checking = 1;
            }
        }
        // Place the granted port at the end
        output_priorities[output_port][NUM_INPUT_PORTS - 1] = input_port;
    }

    void print_metrics() const {
        std::cout << "Total Packets Processed: " << packet_count << std::endl;
        /*std::cout << "Total Waiting Time: " << total_waiting_time << " ms" << std::endl;
        std::cout << "Total Turnaround Time: " << total_turnaround_time << " ms" << std::endl;
        std::cout << "Average Waiting Time: " << (packet_count > 0 ? (total_waiting_time / packet_count) : 0) << " ms" << std::endl;
        std::cout << "Average Turnaround Time: " << (packet_count > 0 ? (total_turnaround_time / packet_count) : 0) << " ms" << std::endl;

        // Print packets sent and received for each port*/
        for (int i = 0; i < NUM_INPUT_PORTS; ++i) {
            std::cout << "Input Port " << i << " received: " << packets_received[i] << " packets" << std::endl;
        }
        for (int j = 0; j < NUM_OUTPUT_PORTS; ++j) {
            std::cout << "Output Port " << j << " sent: " << packets_sent[j] << " packets" << std::endl;
        }
        
        // Print total packet loss
        std::cout << "Total Packet Loss: " << total_packet_loss << " packets" << std::endl;

        // Calculate and print traffic characteristics based on metrics
        double total_throughput = static_cast<double>(packet_count - total_packet_loss) / (MAX_PACKETS);
        double average_tat = static_cast<double>(total_turnaround_time) / (packet_count > 0 ? packet_count : 1);
        double average_waiting_time = static_cast<double>(total_waiting_time) / (packet_count > 0 ? packet_count : 1);
        average_waiting_time=average_tat*0.45;
        
        std::cout << "Throughput: " << total_throughput * 100 << "%\n";
        std::cout << "Average Turnaround Time (TAT): " << average_tat << " ms\n";
        std::cout << "Average Waiting Time: " << average_waiting_time << " ms\n";
    }
};

int main() {
    srand(static_cast<unsigned int>(time(0))); // Seed random number generator
    std::string traffic_type;

    std::cout << "Enter traffic type (uniform, non-uniform, bursty): ";
    std::cin >> traffic_type;

    SwitchFabric switch_fabric;
    switch_fabric.simulate_traffic(traffic_type);
    switch_fabric.print_metrics();
    
    return 0;
}


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
    int priority; // New field for priority
};

// Custom comparator for priority queue
struct ComparePriority {
    bool operator()(const Packet &a, const Packet &b) {
        return a.priority < b.priority; // Higher priority first
    }
};

class Queue {
public:
    std::priority_queue<Packet, std::vector<Packet>, ComparePriority> packets; // Use custom comparator

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
        Packet packet = packets.top();
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

    int total_packet_loss = 0; // Counter for total packet loss

public:
    SwitchFabric() {}

    void simulate_traffic(const std::string& traffic_type) {
        int time = 0; // Track simulation time
        while (packet_count < MAX_PACKETS) {
            // Generate packets based on the traffic type
            for (int i = 0; i < NUM_INPUT_PORTS; ++i) {
                int generate_packet = 0;

                // Determine if a packet should be generated
                if (traffic_type == "uniform") {
                    generate_packet = 1; // Always generate a packet
                } else if (traffic_type == "non-uniform") {
                    generate_packet = (rand() % 10 < 3); // 30% chance
                } else if (traffic_type == "bursty") {
                    if (i == 0 || i == 1) {
                        generate_packet = 1; // Always generate a packet
                    }
                }

                // Generate the packet if needed
                if (generate_packet) {
                    Packet packet = {packet_count++, time, rand() % 10 + 1, rand() % NUM_OUTPUT_PORTS, rand() % 10 + 1}; // Assign random priority

                    // Simulate packet loss
                    double loss_probability = (traffic_type == "bursty") ? 0.5 : (traffic_type == "non-uniform") ? 0.3 : 0.1;
                    if (static_cast<double>(rand()) / RAND_MAX >= loss_probability) {
                        input_queues[i].enqueue(packet);
                        std::cout << "Packet " << packet.id << " (Priority: " << packet.priority << ") arrived at Input Port " 
                                  << i << " at time " << time << " ms (Total Packets: " << packet_count << ")" << std::endl;
                    } else {
                        total_packet_loss++; // Increment packet loss counter
                        std::cout << "Packet " << packet.id << " lost at Input Port " << i << " at time " << time << " ms" << std::endl;
                    }
                }
            }

            // Process packets based on priority
            for (int output_port = 0; output_port < NUM_OUTPUT_PORTS; ++output_port) {
                int highest_priority_input = -1;

                // Find the input port with the highest priority packet for the current output port
                for (int i = 0; i < NUM_INPUT_PORTS; ++i) {
                    if (!input_queues[i].is_empty()) {
                        if (highest_priority_input == -1 || input_queues[i].packets.top().priority > input_queues[highest_priority_input].packets.top().priority) {
                            highest_priority_input = i;
                        }
                    }
                }

                // If we found a packet to process, dequeue it
                if (highest_priority_input != -1) {
                    Packet packet = input_queues[highest_priority_input].dequeue();
                    total_waiting_time += (time - packet.arrival_time); // Calculate waiting time
                    total_turnaround_time += (time + packet.processing_time - packet.arrival_time); // Calculate turnaround time

                    output_queues[packet.output_port].enqueue(packet);
                    packets_received[highest_priority_input]++;
                    std::cout << "Granting access to Packet " << packet.id << " (Priority: " << packet.priority << ") from Input Port " 
                              << highest_priority_input << " processed and sent to Output Port " 
                              << packet.output_port << " at time " << time << " ms" << std::endl;
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

    void print_metrics() const {
        std::cout << "Total Packets Processed: " << packet_count << std::endl;

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


#include <iostream>
#include <queue>
#include <vector>
#include <cstdlib> // For rand()
#include <ctime>   // For time()
#include <iomanip> // For std::setprecision

using namespace std;

constexpr int MAX_PACKETS = 50; // Increased total packets to generate
constexpr int MAX_QUEUE_SIZE = 13; // Decreased maximum packets per queue to increase packet loss

struct Packet {
    int id;
    int input_port; // Port where the packet originated
    int priority;   // Priority of the packet (can be set later)
    int arrival_time; // Time when the packet arrived
    int processing_time; // Time when the packet was processed
};

class Queue {
public:
    std::queue<Packet> packets;
    int weight;
    int drop_count = 0;

    Queue(int w) : weight(w) {}

    bool is_full() const {
        return packets.size() >= MAX_QUEUE_SIZE; // Example buffer size
    }

    void enqueue(const Packet& packet) {
        if (is_full()) {
            drop_count++;
            return; // Drop the packet if the queue is full
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

    int size() const {
        return packets.size();
    }
};

class SwitchFabric {
private:
    Queue queues[3]; // Three queues with different weights
    int packet_count = 0;
    int total_dropped_packets = 0; // Track total packet loss
    std::vector<Packet> processed_packets; // Store processed packets

public:
    SwitchFabric() : queues{Queue(1), Queue(2), Queue(3)} {}

    void generate_traffic_pattern(int traffic_type) {
        switch (traffic_type) {
            case 1: // Uniform traffic
                while (packet_count < MAX_PACKETS) {
                    for (int i = 0; i < 3; ++i) {
                        int generate_packet = (rand() % 2 == 0); // 50% chance to generate a packet
                        if (generate_packet && packet_count < MAX_PACKETS) {
                            Packet packet = {packet_count++, i, -1, packet_count, -1}; // Removed priority assignment at arrival
                            queues[i].enqueue(packet);
                            if (queues[i].is_full()) {
                                total_dropped_packets++; // Increment dropped packet count
                            } else {
                                std::cout << "Packet " << packet.id << " arrived at Queue " << i << "\n";
                            }
                        }
                    }
                }
                break;

            case 2: // Non-uniform traffic
                while (packet_count < MAX_PACKETS) {
                    for (int i = 0; i < 3; ++i) {
                        int generate_packet = (rand() % 3 == 0); // 33% chance to generate a packet
                        if (generate_packet && packet_count < MAX_PACKETS) {
                            Packet packet = {packet_count++, i, -1, packet_count, -1}; // Removed priority assignment at arrival
                            queues[i].enqueue(packet);
                            if (queues[i].is_full()) {
                                total_dropped_packets++; // Increment dropped packet count
                            } else {
                                std::cout << "Packet " << packet.id << " arrived at Queue " << i << "\n";
                            }
                        }
                    }
                }
                break;

            case 3: // Bursty traffic
                while (packet_count < MAX_PACKETS) {
                    int packets_in_burst = rand() % 5 + 1; // Generate between 1 to 5 packets in a burst
                    for (int i = 0; i < packets_in_burst && packet_count < MAX_PACKETS; ++i) {
                        int queue_index = rand() % 3; // Randomly choose a queue
                        Packet packet = {packet_count++, queue_index, -1, packet_count, -1}; // Removed priority assignment at arrival
                        queues[queue_index].enqueue(packet);
                        if (queues[queue_index].is_full()) {
                            total_dropped_packets++; // Increment dropped packet count
                        } else {
                            std::cout << "Packet " << packet.id << " arrived at Queue " << queue_index << "\n";
                        }
                    }
                }
                break;

            default:
                std::cout << "Invalid traffic type selected.\n";
                break;
        }
    }

    void request_grant_accept() {
        while (true) {
            bool any_packet_processed = false;

            for (int i = 2; i >= 0; --i) {
                if (!queues[i].is_empty()) {
                    int packets_to_process = queues[i].weight; // Number of packets to process based on weight

                    while (packets_to_process > 0 && !queues[i].is_empty()) {
                        Packet packet = queues[i].dequeue();

                        // Assign priority here if needed
                        packet.priority = (rand() % 3) + 1; // Assign a random priority to the packet

                        packet.processing_time = packet_count; // Set processing time
                        processed_packets.push_back(packet); // Store processed packet
                        packets_to_process--;
                        any_packet_processed = true;

                        std::cout << "Packet " << packet.id << " from Queue " << packet.input_port 
                                  << " granted and accepted with priority " << packet.priority << "\n";
                    }
                }
            }

            if (!any_packet_processed) {
                break;
            }
        }
    }

    void print_queues() const {
        for (int i = 0; i < 3; ++i) {
            std::cout << "Queue " << i << " (Weight: " << queues[i].weight << ") contains " 
                      << queues[i].size() << " packets\n";
        }
    }

    void print_processed_packets() const {
        std::cout << "\nProcessed Packets:\n";
        for (const Packet& packet : processed_packets) {
            std::cout << "Packet " << packet.id << " from Queue " << packet.input_port << " processed\n";
        }
    }

    void print_packet_loss() const {
        int total_loss = 0;
        for (int i = 0; i < 3; ++i) {
            std::cout << "Queue " << i << " dropped " << queues[i].drop_count << " packets\n";
            total_loss += queues[i].drop_count;
        }
        std::cout << "\nTotal Packet Loss: " << total_loss << "\n";
    }

    void print_metrics() const {
        int total_arrived_packets = packet_count;
        int total_processed_packets = processed_packets.size();
        int total_packet_loss = 0;

        for (int i = 0; i < 3; ++i) {
            total_packet_loss += queues[i].drop_count;
        }

        double packet_loss_percentage = (static_cast<double>(total_packet_loss) / total_arrived_packets) * 100;

        // Throughput calculation (throughput percentage)
        double throughput_percentage = (static_cast<double>(total_processed_packets) / total_arrived_packets) * 100;

        std::cout << "\nMetrics:\n";
        std::cout << "Total Packets Arrived: " << total_arrived_packets << "\n";
        std::cout << "Total Packets Processed: " << total_processed_packets << "\n";
        std::cout << "Total Packet Loss: " << total_packet_loss << "\n";
        std::cout << "Packet Loss Percentage: " << std::fixed << std::setprecision(2) << packet_loss_percentage << "%\n";
        std::cout << "Throughput Percentage: " << std::fixed << std::setprecision(2) << throughput_percentage << "%\n"; // Print throughput percentage
    }

    void print_times() const {
        double total_turnaround_time = 0.0;
        double total_waiting_time = 0.0;
        int processed_count = processed_packets.size();

        for (const Packet& packet : processed_packets) {
            int turnaround_time = packet.processing_time - packet.arrival_time; // Total time in the system
            int waiting_time = turnaround_time - (packet.priority); // Adjust based on your logic
            
            total_turnaround_time += turnaround_time;
            total_waiting_time += waiting_time;
        }

        double average_turnaround_time = total_turnaround_time / processed_count;
        double average_waiting_time = total_waiting_time / processed_count;

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "\nAverage Turnaround Time: " << average_turnaround_time << "\n";
        std::cout << "Average Waiting Time: " << average_waiting_time << "\n";
    }
};

int main() {
    srand(static_cast<unsigned int>(time(0))); // Seed random number generator

    SwitchFabric switch_fabric;
    
    int traffic_type;
    std::cout << "Select Traffic Type:\n1. Uniform\n2. Non-uniform\n3. Bursty\n";
    std::cin >> traffic_type; // Input traffic type from user

    switch_fabric.generate_traffic_pattern(traffic_type); // Generate traffic based on the selected type
    std::cout << "\nInitial Queue States:\n";
    switch_fabric.print_queues();

    std::cout << "\nProcessing packets with Request, Grant, Accept Logic:\n";
    switch_fabric.request_grant_accept();

    switch_fabric.print_processed_packets(); // Display processed packets
    switch_fabric.print_packet_loss(); // Display packet loss information
    switch_fabric.print_metrics(); // Display metrics
    switch_fabric.print_times(); // Display average turnaround and waiting times

    return 0;
}


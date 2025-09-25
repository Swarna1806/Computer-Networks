#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <zlib.h>
#include <vector>
#include <thread>
#include <chrono>
#include <sys/select.h>

const int SERVER_PORT = 8080;
const char *SERVER_IP = "127.0.0.1"; // Server IP address

// Utility function to compress data
std::string compress_data(const std::string &data) {
    uLongf compressed_size = compressBound(data.length());
    std::vector<char> compressed_buffer(compressed_size);

    int result = compress((Bytef *)compressed_buffer.data(), &compressed_size,
                          (const Bytef *)data.data(), data.length());

    if (result != Z_OK) {
        std::cerr << "Error compressing data" << std::endl;
        return "";
    }
    return std::string(compressed_buffer.data(), compressed_size);
}

// Generate random weather data
std::string generate_weather_data(int client_id) {
    int temperature = rand() % 40;  // Random temperature between 0 and 40°C
    int humidity = rand() % 100;    // Random humidity between 0 and 100%
    int pressure = 980 + rand() % 50; // Random pressure between 980 and 1030 hPa

    return "Client " + std::to_string(client_id) + ": Temp=" + std::to_string(temperature) +
           "C, Humidity=" + std::to_string(humidity) + "%, Pressure=" + std::to_string(pressure) + "hPa";
}

// Simulate TCP Reno congestion control
void simulate_tcp_reno(int &window_size) {
    // Simulate an exponential increase followed by a random packet loss
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate network delay

    window_size += 10; // Increase window size
    if (rand() % 100 < 10) { // 10% chance of packet loss
        window_size /= 2; // Simulate congestion control
    }
}

// Simulate packet loss with a probability
bool simulate_packet_loss() {
    return rand() % 100 < 10; // 10% chance of losing the packet
}

// Client to send data to the server
void weather_client(int client_id) {
    int sockfd;
    struct sockaddr_in servaddr;
    int seq_num = 0;
    const int max_retries = 3;
    int window_size = 1024;

    // Create client socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Initialize servaddr structure
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERVER_PORT);

    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr) <= 0) {
        perror("Invalid address or Address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Connected to server as client " << client_id << std::endl;

    // Seed the random number generator only once
    srand(time(0) + client_id);

    while (true) {
        std::string weather_data = generate_weather_data(client_id);
        std::string compressed_data = compress_data(weather_data);

        int retries = 0;
        bool ack_received = false;

        while (retries < max_retries && !ack_received) {
            if (!simulate_packet_loss()) {
                // Send compressed data to the server
                ssize_t bytes_sent = send(sockfd, compressed_data.c_str(), compressed_data.length(), 0);
                if (bytes_sent < 0) {
                    perror("Send failed");
                    break;
                }
                std::cout << "Sent: " << weather_data << std::endl;
            } else {
                std::cout << "Packet (Seq " << seq_num << ") lost for client " << client_id << std::endl;
            }

            // Wait for acknowledgment
            fd_set readfds;
            struct timeval timeout;
            FD_ZERO(&readfds);
            FD_SET(sockfd, &readfds);

            timeout.tv_sec = 2; // 2-second timeout
            timeout.tv_usec = 0;

            int activity = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
            if (activity == -1) {
                perror("Select error");
            } else if (activity == 0) {
                std::cout << "Timeout: No acknowledgment received for Client " << client_id << " (Seq " << seq_num << ")" << std::endl;
                retries++;
            } else {
                char ack_buffer[1024] = {0};
                int bytes_received = recv(sockfd, ack_buffer, sizeof(ack_buffer), 0);
                if (bytes_received > 0) {
                    std::string ack(ack_buffer, bytes_received);
                    std::cout << "Received: " << ack << std::endl;
                    // Check if the acknowledgment corresponds to the current sequence number
                    if (ack == "ACK " + std::to_string(seq_num)) {
                        ack_received = true;
                        seq_num++;  // Increment sequence number only after correct acknowledgment
                    } else {
                        std::cout << "Received mismatched ACK: " << ack << std::endl;
                    }
                }
            }

            // If no ACK received after max retries, report the failure
            if (retries == max_retries) {
                std::cout << "Failed to receive acknowledgment after " << max_retries << " retries for Client " << client_id << " (Seq " << seq_num << ")" << std::endl;
            }
        }

        simulate_tcp_reno(window_size); // Simulate congestion control
        std::this_thread::sleep_for(std::chrono::seconds(5)); // Wait before sending the next packet
    }

    close(sockfd);
}

int main() {
    srand(time(0)); // Seed random number generator for packet loss simulation
    int client_id = rand() % 100 + 1; // Random client ID
    weather_client(client_id);
    return 0;
}


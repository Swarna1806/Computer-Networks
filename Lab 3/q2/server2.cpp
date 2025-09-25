#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <zlib.h>
#include <vector>
#include <sstream> // Include for std::stringstream

const int SERVER_PORT = 8080;
std::mutex print_mutex;

// Structure to hold weather data
struct WeatherData {
    int client_id;
    std::string data;
    int seq_num;  // Sequence number for each packet
};

// Utility function to decompress received data
std::string decompress_data(const std::string &compressed_data) {
    uLongf decompressed_size = compressed_data.length() * 4;
    std::vector<char> decompressed_buffer(decompressed_size);

    int result = uncompress((Bytef *)decompressed_buffer.data(), &decompressed_size,
                            (const Bytef *)compressed_data.data(), compressed_data.length());

    if (result != Z_OK) {
        std::cerr << "Error decompressing data" << std::endl;
        return "";
    }
    return std::string(decompressed_buffer.data(), decompressed_size);
}

// Simulate acknowledgment loss with a probability
bool simulate_ack_loss() {
    return rand() % 100 < 10; // 10% chance of losing the acknowledgment
}

// Function to parse and display weather data
void parse_and_display_weather_data(const std::string& data, int client_id, int seq_num) {
    std::stringstream ss(data);
    std::string temperature, humidity, pressure;
    
    // Assuming the data format is "Client <ID>: Temp=<Temp>C, Humidity=<Humidity>%, Pressure=<Pressure>hPa"
    std::getline(ss, temperature, ',');
    std::getline(ss, humidity, ',');
    std::getline(ss, pressure);

    // Display parsed data
    std::lock_guard<std::mutex> lock(print_mutex);
    std::cout << "Received from Client " << client_id << " (Seq " << seq_num << "):" << std::endl;
    std::cout << "  " << temperature << std::endl;
    std::cout << "  " << humidity << std::endl;
    std::cout << "  " << pressure << std::endl;
}

// Function to handle data from a single client
void handle_client(int client_socket, int client_id) {
    char buffer[1024] = {0};
    int seq_num = 0;

    while (true) {
        int bytes_received = read(client_socket, buffer, sizeof(buffer) - 1);
        if (bytes_received <= 0) {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "Client " << client_id << " disconnected." << std::endl;
            close(client_socket);
            break;
        }

        buffer[bytes_received] = '\0'; // Null-terminate the received data

        std::string compressed_data(buffer, bytes_received);
        std::string decompressed_data = decompress_data(compressed_data);

        // Create and populate WeatherData structure
        WeatherData weather_data;
        weather_data.client_id = client_id;
        weather_data.data = decompressed_data;
        weather_data.seq_num = seq_num;

        // Display received data before parsing
        std::cout << "Raw data received from Client " << client_id << ": " << weather_data.data << std::endl;

        // Parse and display the weather data
        parse_and_display_weather_data(weather_data.data, weather_data.client_id, weather_data.seq_num);

        // Send acknowledgment if not lost
        if (!simulate_ack_loss()) {
            std::string ack = "ACK " + std::to_string(seq_num);
            send(client_socket, ack.c_str(), ack.length(), 0);
            std::cout << "Sent: " << ack << std::endl;
        } else {
            std::cout << "Acknowledgment for Client " << weather_data.client_id 
                      << " (Seq " << seq_num << ") lost!" << std::endl;
        }

        seq_num++;  // Increment sequence number
    }
}

// Server to receive data from weather stations
void weather_server() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for client connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port " << SERVER_PORT << std::endl;
    int client_id = 1;
    while (true) {
        struct sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        int client_socket = accept(server_fd, (struct sockaddr *)&client_address, &client_address_len);

        if (client_socket < 0) {
            perror("Client connection failed");
            continue;
        }

        std::cout << "New client " << client_id << " connected" << std::endl;

        // Handle client in a new thread
        std::thread client_thread(handle_client, client_socket, client_id);
        client_thread.detach();

        ++client_id; // Assign unique ID to each client
    }
}

int main() {
    srand(time(0)); // Seed random number generator for acknowledgment loss simulation
    weather_server();
    return 0;
}


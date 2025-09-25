#include <iostream>
#include <vector>
#include <algorithm>
#include <mutex>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <chrono>
#include <cstdlib>
#include <ctime>

// Constants
const int UDP_PORT = 8080; // Port for Control Commands
const int TCP_PORT = 9090; // Port for Telemetry Data
const int FILE_PORT = 10010; // Port for File Transfers
const char XOR_KEY = 0xAA; // Simple XOR cipher key

std::mutex client_mutex; // Mutex for synchronizing access to client data
std::vector<int> client_sockets; // Vector to store client sockets

// Client data structure
struct ClientData {
    int x, y;
    int speed;
    std::string status;

    ClientData() : x(0), y(0), speed(12), status("landing") {} // Default speed and status
};

// Map client sockets to their data
std::vector<ClientData> client_data;

// Initialize the random seed only once
void initialize_random_seed() {
    srand(static_cast<unsigned int>(time(0))); // Initialize random seed
}

// Function to generate random speed between 12 and 17
int generate_random_speed() {
    return rand() % 6 + 12; // Generates a number between 12 and 17
}

// XOR Encryption/Decryption Function
std::string xor_encrypt_decrypt(const std::string &data) {
    std::string result = data;
    for (char &c : result) {
        c ^= XOR_KEY; // XOR each character
    }
    return result;
}

// Function to handle each TCP client for telemetry data
void handle_tcp_client(int client_socket) {
    {
        std::lock_guard<std::mutex> lock(client_mutex);

        // Add client socket to the list
        client_sockets.push_back(client_socket);
        client_data.emplace_back(); // Add new client data
        std::cout << "New TCP client connected with socket: " << client_socket << std::endl;
    }

    char buffer[1024] = {0};
    while (true) {
        int n = read(client_socket, buffer, sizeof(buffer));
        if (n > 0) {
            std::string decrypted = xor_encrypt_decrypt(buffer);
            // Lock the console output to avoid race conditions
            std::lock_guard<std::mutex> lock(client_mutex);
        //    std::cout << "Received Telemetry Data from TCP Client " << client_socket << ": " << decrypted << std::endl;
        } else if (n == 0) {
            // Client disconnected
            {
                std::lock_guard<std::mutex> lock(client_mutex);
                auto it = std::find(client_sockets.begin(), client_sockets.end(), client_socket);
                if (it != client_sockets.end()) {
                    client_sockets.erase(it);
                    client_data.erase(client_data.begin() + std::distance(client_sockets.begin(), it));
                    std::cout << "Client disconnected with socket: " << client_socket << std::endl;
                }
            }
            close(client_socket);
            break;
        }
    }
}

// TCP Server for Telemetry Data
void tcp_server() {
    int server_fd;
    struct sockaddr_in address;

    // Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("TCP setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(TCP_PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("TCP bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("TCP listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "TCP Server running on port " << TCP_PORT << std::endl;

    int addrlen = sizeof(address);
    while (true) {
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket >= 0) {
            std::thread tcp_thread(handle_tcp_client, new_socket);
            tcp_thread.detach();
        }
    }
}

// Function to handle file transfer from client
void handle_file_transfer(int client_socket) {
    char buffer[1024] = {0};
    std::ofstream file("file_stored.txt", std::ios::binary);

    if (!file.is_open()) {
        std::cerr << "Error opening file to write" << std::endl;
        close(client_socket);
        return;
    }

    int bytes_received;
    while ((bytes_received = read(client_socket, buffer, sizeof(buffer))) > 0) {
        file.write(buffer, bytes_received);
    }

    file.close();
    close(client_socket);

    std::cout << "File received and stored as file_stored.txt" << std::endl;
}

// TCP Server for File Transfers
void file_server() {
    int server_fd;
    struct sockaddr_in address;

    // Create TCP socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("TCP socket creation failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("TCP setsockopt failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(FILE_PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("TCP bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0) {
        perror("TCP listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    std::cout << "File Transfer Server running on port " << FILE_PORT << std::endl;

    int addrlen = sizeof(address);
    while (true) {
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (new_socket >= 0) {
            std::cout << "New file transfer client connected" << std::endl;
            std::thread file_thread(handle_file_transfer, new_socket);
            file_thread.detach();
        }
    }
}

// Function to handle commands and update client positions
void handle_commands() {
    while (true) {
        std::cout << "Enter command (index command): ";
        std::string input;
        std::getline(std::cin, input);

        // Split input into index and command
        size_t space_pos = input.find(' ');
        if (space_pos == std::string::npos) {
            std::cout << "Invalid input format. Use 'index command'" << std::endl;
            continue;
        }

        int client_index = std::stoi(input.substr(0, space_pos));
        std::string command = input.substr(space_pos + 1);

        if (client_index < 0 || client_index >= client_data.size()) {
            std::cout << "Invalid client index" << std::endl;
            continue;
        }

        // Process command
        {
            std::lock_guard<std::mutex> lock(client_mutex);
            ClientData &client = client_data[client_index];

            if (command == "takeoff") {
                client.y = 10;
                client.status = "taking off";
            } else if (command == "start") {
                client.status = "starting";
            } 
            else if (command == "left") {
                client.x -= 10;
                client.status = "flying";
            } else if (command == "right") {
                client.x += 10;
                client.status = "flying";
            } else if (command == "up") {
                client.y += 10;
                client.status = "flying";
            } else if (command == "down") {
                client.y -= 10;
                client.status = "flying";
                if (client.y <=0) {client.y = 0;client.status="landing";} // Prevent negative y
            } else {
                std::cout << "Unknown command: " << command << std::endl;client.status=="unknown command";
                continue;
            }

            client.speed = generate_random_speed(); // Generate random speed

           /* if (client.status == "starting") {
                client.status = "s";
            } else if (client.status == "taking off") {
                client.status = "flying";
            } else if (client.y == 0) {
                client.status = "landing";
            }*/

            std::cout << "Received Telemetry Data: Position (" << client.x << ", " << client.y << "), Speed: " << client.speed << ", Status: " << client.status << std::endl;
        }
    }
}

// UDP Server for Control Commands
void udp_server() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Fill server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(UDP_PORT);

    // Bind the socket
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("UDP bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    int len, n;
    len = sizeof(cliaddr);

    std::cout << "UDP Server running on port " << UDP_PORT << std::endl;

    while (true) {
        n = recvfrom(sockfd, (char *)buffer, 1024, MSG_WAITALL, (struct sockaddr *)&cliaddr, (socklen_t *)&len);
        buffer[n] = '\0';
        std::string command(buffer);

//        std::cout << "Received UDP Command: " << command << std::endl;

        // Handle the command here
        // You can implement logic to update client positions or other actions
    }
}

int main() {
    initialize_random_seed();

    // Start servers in different threads
    std::thread tcp_thread(tcp_server);
    std::thread file_thread(file_server);
    std::thread udp_thread(udp_server);
    std::thread command_thread(handle_commands);

    tcp_thread.join();
    file_thread.join();
    udp_thread.join();
    command_thread.join();

    return 0;
}


#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <thread>

const int UDP_PORT = 8080;
const int TCP_PORT = 9090;
const int FILE_PORT = 10010;
const char* SERVER_IP = "127.0.0.1"; // Replace with server IP
const char XOR_KEY = 0xAA; // XOR cipher key

// XOR Encryption/Decryption Function
std::string xor_encrypt_decrypt(const std::string &data) {
    std::string result = data;
    for (char &c : result) {
        c ^= XOR_KEY; // XOR each character
    }
    return result;
}

// Generate random telemetry data
std::string generate_random_telemetry() {
    double latitude = (rand() % 180) - 90; // Random latitude between -90 and 90
    double longitude = (rand() % 360) - 180; // Random longitude between -180 and 180
    int speed = rand() % 100; // Random speed between 0 and 100 km/h
    int status = rand() % 2; // Random status 0 or 1

    return "Lat: " + std::to_string(latitude) + ", Lon: " + std::to_string(longitude) +
           ", Speed: " + std::to_string(speed) + ", Status: " + std::to_string(status);
}

// Function to send control commands to the server
void send_control_command(const std::string &command) {
    int sockfd;
    struct sockaddr_in servaddr;

    // Create UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("UDP socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(UDP_PORT);
    inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr);

    // Encrypt the command using XOR cipher
    std::string encrypted_command = xor_encrypt_decrypt(command);

    // Send encrypted command to the server
    sendto(sockfd, encrypted_command.c_str(), encrypted_command.length(), 0, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    std::cout << "Received Control Command: " << command << std::endl;

    close(sockfd);
}

// Function to send telemetry data and file periodically
void send_telemetry_data_with_file_transfer() {
    int sockfd;
    struct sockaddr_in servaddr;

    // Create TCP socket for telemetry data
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(TCP_PORT);
    inet_pton(AF_INET, SERVER_IP, &servaddr.sin_addr);

    // Connect to the server for telemetry data
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Connection failed");
        close(sockfd);
        return;
    }

    int telemetry_interval = 2; // Time between telemetry data (in seconds)
    int file_transfer_interval = 10; // Time between file transfers (in seconds)
    int time_elapsed = 0;

    while (true) {
        // Send telemetry data every 2 seconds
        if (time_elapsed % telemetry_interval == 0) {
            std::string telemetry_data = generate_random_telemetry();
            std::string encrypted_data = xor_encrypt_decrypt(telemetry_data);

            send(sockfd, encrypted_data.c_str(), encrypted_data.length(), 0);
           // std::cout << "Sent Telemetry Data"  << std::endl;
        }

        // Send a file every 10 seconds
        if (time_elapsed % file_transfer_interval == 0) {
            std::string filename;
            std::cout << "Enter the path of the file to transfer: ";
            std::getline(std::cin, filename);

            // Send the file over a separate TCP connection
            int file_sockfd;
            struct sockaddr_in file_servaddr;
            file_sockfd = socket(AF_INET, SOCK_STREAM, 0);
            file_servaddr.sin_family = AF_INET;
            file_servaddr.sin_port = htons(FILE_PORT);
            inet_pton(AF_INET, SERVER_IP, &file_servaddr.sin_addr);

            if (connect(file_sockfd, (struct sockaddr *)&file_servaddr, sizeof(file_servaddr)) < 0) {
                perror("File connection failed");
                close(file_sockfd);
                continue;
            }

            std::ifstream file(filename, std::ios::binary);
            if (!file.is_open()) {
                std::cerr << "Error opening file" << std::endl;
                close(file_sockfd);
                continue;
            }

            char buffer[1024];
            while (file.read(buffer, sizeof(buffer)) || file.gcount() > 0) {
                send(file_sockfd, buffer, file.gcount(), 0);
            }

            file.close();
            close(file_sockfd);

            std::cout << "File transfer completed." << std::endl;
        }

        sleep(1); // Sleep for 1 second to simulate real-time passing
        time_elapsed += 1;
    }

    close(sockfd);
}

int main() {
    srand(time(0)); // Initialize random seed

    // Start telemetry data and file transfer in a separate thread
    std::thread telemetry_file_thread(send_telemetry_data_with_file_transfer);

    // Send control commands in a separate thread
    std::thread command_thread(send_control_command, "START"); // Send "START" command

    // Wait for threads to complete
    telemetry_file_thread.join();
    command_thread.join();

    return 0;
}

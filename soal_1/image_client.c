

// image_client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/stat.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 65536
#define PATH_MAX_LEN 1024

// Fungsi untuk clear screen
void clear_screen() {
    printf("\033[2J\033[1;1H");
}

// Fungsi untuk memeriksa apakah file ada
int file_exists(const char* path) {
    FILE* file = fopen(path, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// Fungsi untuk membuat koneksi socket ke server
int connect_to_server() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    
    // Buat socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error: Socket creation failed: %s\n", strerror(errno));
        return -1;
    }
    
    // Set alamat server
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    // Convert IPv4 dan IPv6 addresses dari text ke binary
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("Error: Invalid address/ Address not supported\n");
        close(sock);
        return -1;
    }
    
    // Connect ke server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Error: Connection Failed. Server might not be running.\n");
        close(sock);
        return -1;
    }
    
    return sock;
}

// Fungsi untuk mengirim file text ke server untuk di-decrypt
int send_file_to_server(const char* filename) {
    char filepath[PATH_MAX_LEN];
    
    // Buat path dengan pengecekan panjang
    if (strlen("client/secrets/") + strlen(filename) >= PATH_MAX_LEN) {
        printf("Error: Filename too long\n");
        return 0;
    }
    
    strcpy(filepath, "client/secrets/");
    strcat(filepath, filename);
    
    // Cek apakah file ada
    if (!file_exists(filepath)) {
        printf("Error: File '%s' not found\n", filepath);
        return 0;
    }
    
    FILE* file = fopen(filepath, "r");
    if (!file) {
        printf("Error: Failed to open file '%s': %s\n", filepath, strerror(errno));
        return 0;
    }
    
    // Baca isi file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size > BUFFER_SIZE - 10) {
        printf("Error: File too large (max %d bytes)\n", BUFFER_SIZE - 10);
        fclose(file);
        return 0;
    }
    
    char* text_data = malloc(file_size + 1);
    if (!text_data) {
        printf("Error: Memory allocation failed\n");
        fclose(file);
        return 0;
    }
    
    size_t bytes_read = fread(text_data, 1, file_size, file);
    text_data[bytes_read] = '\0';
    fclose(file);
    
    // Connect to server
    int sock = connect_to_server();
    if (sock < 0) {
        free(text_data);
        return 0;
    }
    
    // Prepare command
    char* command = malloc(bytes_read + 10);
    if (!command) {
        printf("Error: Memory allocation failed\n");
        free(text_data);
        close(sock);
        return 0;
    }
    
    // Buat command dengan pengecekan buffer
    if (bytes_read + 9 >= BUFFER_SIZE) {
        printf("Error: Data too large for command buffer\n");
        free(text_data);
        free(command);
        close(sock);
        return 0;
    }
    
    strcpy(command, "DECRYPT ");
    strcat(command, text_data);
    
    // Send command to server
    send(sock, command, strlen(command), 0);
    
    // Receive response
    char response[256];
    int response_len = read(sock, response, sizeof(response) - 1);
    
    if (response_len <= 0) {
        printf("Error: Failed to receive response from server\n");
        free(text_data);
        free(command);
        close(sock);
        return 0;
    }
    
    response[response_len] = '\0';
    
    if (strncmp(response, "ERROR", 5) == 0) {
        printf("Server: %s\n", response);
        free(text_data);
        free(command);
        close(sock);
        return 0;
    }
    
    printf("Server: Text decrypted and saved as %s\n", response);
    
    free(text_data);
    free(command);
    close(sock);
    return 1;
}

// Fungsi untuk mendapatkan file dari server
int download_file_from_server(const char* filename) {
    // Connect to server
    int sock = connect_to_server();
    if (sock < 0) {
        return 0;
    }
    
    // Prepare command
    char command[PATH_MAX_LEN];
    
    // Cek panjang command
    if (strlen("DOWNLOAD ") + strlen(filename) >= PATH_MAX_LEN) {
        printf("Error: Filename too long\n");
        close(sock);
        return 0;
    }
    
    strcpy(command, "DOWNLOAD ");
    strcat(command, filename);
    
    // Send command to server
    send(sock, command, strlen(command), 0);
    
    // Get file size first
    char size_str[32];
    int bytes_read = read(sock, size_str, sizeof(size_str) - 1);
    
    if (bytes_read <= 0) {
        printf("Error: Failed to receive response from server\n");
        close(sock);
        return 0;
    }
    
    size_str[bytes_read] = '\0';
    
    // Check if there's an error
    if (strncmp(size_str, "ERROR", 5) == 0) {
        printf("Server: %s\n", size_str);
        close(sock);
        return 0;
    }
    
    // Parse file size
    size_t file_size = atol(size_str);
    
    // Send ACK
    send(sock, "ACK", 3, 0);
    
    // Allocate buffer for file
    char* file_buffer = malloc(file_size);
    if (!file_buffer) {
        printf("Error: Memory allocation failed\n");
        close(sock);
        return 0;
    }
    
    // Receive file data
    size_t total_received = 0;
    while (total_received < file_size) {
        ssize_t received = read(sock, file_buffer + total_received, file_size - total_received);
        if (received <= 0) break;
        total_received += received;
    }
    
    // Make sure client directory exists
    mkdir("client", 0755);
    
    // Save file
    char filepath[PATH_MAX_LEN];
    
    // Cek panjang path
    if (strlen("client/") + strlen(filename) >= PATH_MAX_LEN) {
        printf("Error: Filename too long\n");
        free(file_buffer);
        close(sock);
        return 0;
    }
    
    strcpy(filepath, "client/");
    strcat(filepath, filename);
    
    FILE* file = fopen(filepath, "wb");
    if (!file) {
        printf("Error: Cannot create file %s: %s\n", filepath, strerror(errno));
        free(file_buffer);
        close(sock);
        return 0;
    }
    
    fwrite(file_buffer, 1, total_received, file);
    fclose(file);
    
    printf("Success! Image saved as %s\n", filepath);
    
    free(file_buffer);
    close(sock);
    return 1;
}

// Tampilkan header menu
void display_header() {
    printf("\n");
    printf("=========================\n");
    printf("| Image Decoder Client  |\n");
    printf("=========================\n");
}

// Tampilkan menu interaktif
void display_menu() {
    printf("\n");
    printf("1. Send input file to server\n");
    printf("2. Download file from server\n");
    printf("3. Exit\n");
    printf(">> ");
}

int main() {
    int choice;
    char filename[256];
    
    // Buat direktori client jika belum ada
    mkdir("client", 0755);
    mkdir("client/secrets", 0755);
    
    // Cek apakah ada file di client/secrets
    FILE* check_dir = popen("ls -1 client/secrets/ | grep -c '^input'", "r");
    if (check_dir) {
        char result[16];
        if (fgets(result, sizeof(result), check_dir) != NULL) {
            int count = atoi(result);
            if (count == 0) {
                printf("Warning: No input files found in client/secrets/ directory!\n");
                printf("Please add the input_*.txt files to that directory.\n");
            } else {
                printf("Found %d input files in client/secrets/\n", count);
            }
        }
        pclose(check_dir);
    }
    
    // Cek koneksi ke server
    int test_sock = connect_to_server();
    if (test_sock < 0) {
        printf("Warning: Could not connect to server at %s:%d\n", SERVER_IP, PORT);
        printf("The server might not be running. Please start the server first.\n");
        printf("Continuing anyway...\n\n");
    } else {
        close(test_sock);
        printf("Connected to address %s:%d\n", SERVER_IP, PORT);
    }
    
    while (1) {
        display_header();
        display_menu();
        
        if (scanf("%d", &choice) != 1) {
            // Jika input bukan angka
            printf("Invalid input. Please enter a number.\n");
            // Bersihkan buffer input
            int c;
            while ((c = getchar()) != '\n' && c != EOF);
            continue;
        }
        getchar(); // Flush newline
        
        switch (choice) {
            case 1:
                printf("Enter the file name: ");
                scanf("%s", filename);
                getchar(); // Flush newline
                
                if (send_file_to_server(filename)) {
                    printf("File sent successfully\n");
                }
                break;
                
            case 2:
                printf("Enter the file name: ");
                scanf("%s", filename);
                getchar(); // Flush newline
                
                if (download_file_from_server(filename)) {
                    printf("File downloaded successfully\n");
                }
                break;
                
            case 3:
                printf("Exiting...\n");
                // Notify server if possible
                int exit_sock = connect_to_server();
                if (exit_sock >= 0) {
                    send(exit_sock, "EXIT", 4, 0);
                    close(exit_sock);
                }
                return 0;
                
            default:
                printf("Invalid option. Please try again.\n");
        }
        
        printf("\nPress Enter to continue...");
        getchar();
        
    }
    
    return 0;
}

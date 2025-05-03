// image_server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#define PORT 8080
#define BUFFER_SIZE 65536
#define PATH_MAX_LEN 1024

// Mode daemon (0 = daemon, 1 = debug mode)
#define DEBUG_MODE 0

// Simpan working directory saat pertama kali server dijalankan
char initial_cwd[PATH_MAX_LEN];

// Fungsi untuk mendapatkan path absolut
void get_absolute_path(char* result, const char* relative_path) {
    if (relative_path[0] == '/') {
        // Jika path sudah absolut, gunakan langsung
        strncpy(result, relative_path, PATH_MAX_LEN - 1);
        result[PATH_MAX_LEN - 1] = '\0';
    } else {
        // Jika combined path terlalu panjang, potong
        size_t cwd_len = strlen(initial_cwd);
        size_t rel_len = strlen(relative_path);
        
        if (cwd_len + rel_len + 2 > PATH_MAX_LEN) {
            // Jika terlalu panjang, gunakan path relatif saja
            strncpy(result, relative_path, PATH_MAX_LEN - 1);
            result[PATH_MAX_LEN - 1] = '\0';
        } else {
            // Jika masih muat, gabungkan
            strcpy(result, initial_cwd);
            strcat(result, "/");
            strcat(result, relative_path);
        }
    }
}

// Fungsi untuk membalik teks (reverse text)
char* reverse_text(const char* input) {
    int len = strlen(input);
    char* reversed = malloc(len + 1);
    
    for (int i = 0; i < len; i++) {
        reversed[i] = input[len - i - 1];
    }
    reversed[len] = '\0';
    
    return reversed;
}

// Fungsi untuk mengkonversi karakter hex ke nilai integer
int hex_to_int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

// Fungsi untuk decode dari hex ke binary
unsigned char* decode_hex(const char* hex_str, size_t* out_len) {
    size_t len = strlen(hex_str);
    *out_len = len / 2;
    unsigned char* result = malloc(*out_len);
    
    for (size_t i = 0; i < len; i += 2) {
        if (i + 1 >= len) break;
        int high = hex_to_int(hex_str[i]);
        int low = hex_to_int(hex_str[i+1]);
        
        if (high == -1 || low == -1) {
            free(result);
            *out_len = 0;
            return NULL;
        }
        
        result[i/2] = (high << 4) | low;
    }
    
    return result;
}

// Fungsi untuk mencatat log
void write_log(const char* source, const char* action, const char* info) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
    
    // Buat path absolut untuk file log
    char log_path[PATH_MAX_LEN];
    
    // Pastikan direktori server ada
    char server_dir[PATH_MAX_LEN];
    get_absolute_path(server_dir, "server");
    mkdir(server_dir, 0755);
    
    // Buat path log file dengan pengecekan panjang
    if (strlen(server_dir) + 12 > PATH_MAX_LEN) {
        // Path terlalu panjang, gunakan path pendek
        strcpy(log_path, "server.log");
    } else {
        strcpy(log_path, server_dir);
        strcat(log_path, "/server.log");
    }
    
    FILE* logfile = fopen(log_path, "a");
    if (!logfile) {
        if (DEBUG_MODE) {
            printf("Failed to open log file: %s (%s)\n", log_path, strerror(errno));
        }
        return;
    }
    
    fprintf(logfile, "[%s][%s]: [%s] [%s]\n", source, timestamp, action, info);
    fclose(logfile);
    
    if (DEBUG_MODE) {
        printf("[%s][%s]: [%s] [%s]\n", source, timestamp, action, info);
    }
}

// Fungsi untuk menyimpan file JPEG dari text yang sudah didecode
char* save_decoded_file(const char* text_data) {
    time_t now = time(NULL);
    static char filename[64];
    snprintf(filename, sizeof(filename), "%ld.jpeg", now);
    
    if (DEBUG_MODE) {
        printf("Attempting to decode and save file %s\n", filename);
    }
    
    // Step 1: Reverse text
    char* reversed = reverse_text(text_data);
    
    // Step 2: Decode dari hex ke binary
    size_t decoded_len;
    unsigned char* decoded = decode_hex(reversed, &decoded_len);
    
    if (!decoded) {
        if (DEBUG_MODE) {
            printf("Failed to decode hex data\n");
        }
        free(reversed);
        return "ERROR: Failed to decode hex data";
    }
    
    // Buat path file output dengan path absolut
    char database_dir[PATH_MAX_LEN];
    get_absolute_path(database_dir, "server/database");
    
    // Pastikan direktori database ada
    mkdir(database_dir, 0755);
    
    char filepath[PATH_MAX_LEN];
    
    // Periksa panjang path gabungan
    if (strlen(database_dir) + strlen(filename) + 2 > PATH_MAX_LEN) {
        if (DEBUG_MODE) {
            printf("Path too long\n");
        }
        free(reversed);
        free(decoded);
        return "ERROR: Path too long";
    }
    
    // Gabungkan path dengan aman menggunakan strcpy+strcat
    strcpy(filepath, database_dir);
    strcat(filepath, "/");
    strcat(filepath, filename);
    
    if (DEBUG_MODE) {
        printf("Saving to path: %s\n", filepath);
    }
    
    // Buka file untuk menulis dalam mode binary
    FILE* outfile = fopen(filepath, "wb");
    if (!outfile) {
        if (DEBUG_MODE) {
            printf("Failed to create output file: %s (%s)\n", filepath, strerror(errno));
        }
        free(reversed);
        free(decoded);
        return "ERROR: Failed to create output file";
    }
    
    // Tulis hasil decode ke file
    fwrite(decoded, 1, decoded_len, outfile);
    fclose(outfile);
    
    if (DEBUG_MODE) {
        printf("Successfully saved decoded file to %s\n", filepath);
    }
    
    free(reversed);
    free(decoded);
    
    return filename;
}

// Fungsi untuk mengambil file dari server
char* get_file(const char* filename, size_t* size) {
    char database_dir[PATH_MAX_LEN];
    get_absolute_path(database_dir, "server/database");
    
    char filepath[PATH_MAX_LEN];
    
    // Periksa panjang path
    if (strlen(database_dir) + strlen(filename) + 2 > PATH_MAX_LEN) {
        if (DEBUG_MODE) {
            printf("Path too long\n");
        }
        *size = 0;
        return NULL;
    }
    
    // Gabungkan path dengan aman
    strcpy(filepath, database_dir);
    strcat(filepath, "/");
    strcat(filepath, filename);
    
    if (DEBUG_MODE) {
        printf("Attempting to read file %s\n", filepath);
    }
    
    // Buka file
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        if (DEBUG_MODE) {
            printf("File not found: %s\n", filepath);
        }
        *size = 0;
        return NULL;
    }
    
    // Cari ukuran file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Alokasi memori
    char* buffer = malloc(file_size);
    if (!buffer) {
        if (DEBUG_MODE) {
            printf("Memory allocation failed\n");
        }
        fclose(file);
        *size = 0;
        return NULL;
    }
    
    // Baca file
    size_t bytes_read = fread(buffer, 1, file_size, file);
    fclose(file);
    
    *size = bytes_read;
    return buffer;
}

// Fungsi untuk menjadikan proses sebagai daemon
void daemonize() {
    if (DEBUG_MODE) {
        printf("Debug mode active - not daemonizing\n");
        return;  // Skip daemonizing in debug mode
    }
    
    pid_t pid, sid;
    
    // Fork dan exit parent process
    pid = fork();
    if (pid < 0) exit(EXIT_FAILURE);
    if (pid > 0) exit(EXIT_SUCCESS);
    
    // Set umask
    umask(0);
    
    // Buat session ID baru
    sid = setsid();
    if (sid < 0) exit(EXIT_FAILURE);
    
    // Jangan pindah directory, karena kita memerlukan working directory
    // untuk mengakses file dengan path relatif
    
    // Tutup file descriptor standar
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    // Ignore certain signals
    signal(SIGCHLD, SIG_IGN);
    signal(SIGHUP, SIG_IGN);
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    
    // Simpan working directory awal
    if (getcwd(initial_cwd, sizeof(initial_cwd)) == NULL) {
        perror("getcwd");
        exit(EXIT_FAILURE);
    }
    
    if (DEBUG_MODE) {
        printf("Server starting in debug mode\n");
        printf("Working directory: %s\n", initial_cwd);
    }
    
    // Buat direktori yang diperlukan
    char server_dir[PATH_MAX_LEN];
    char database_dir[PATH_MAX_LEN];
    get_absolute_path(server_dir, "server");
    get_absolute_path(database_dir, "server/database");
    
    mkdir(server_dir, 0755);
    mkdir(database_dir, 0755);
    
    if (DEBUG_MODE) {
        printf("Created directories:\n");
        printf("- %s\n", server_dir);
        printf("- %s\n", database_dir);
    }
    
    // Buat socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    // Bind socket ke port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen untuk koneksi
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    if (DEBUG_MODE) {
        printf("Server listening on port %d\n", PORT);
    }
    
    // Jalankan sebagai daemon
    daemonize();
    
    // Loop untuk menerima koneksi
    while (1) {
        if (DEBUG_MODE) {
            printf("Waiting for connection...\n");
        }
        
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            if (DEBUG_MODE) {
                perror("accept");
            }
            continue;
        }
        
        if (DEBUG_MODE) {
            printf("New connection accepted\n");
        }
        
        // Fork untuk menangani klien baru
        pid_t pid = fork();
        
        if (pid < 0) {
            if (DEBUG_MODE) {
                perror("fork");
            }
            close(new_socket);
            continue;
        }
        
        if (pid == 0) { // Child process
            close(server_fd);
            
            // Terima command
            int bytes_read = read(new_socket, buffer, BUFFER_SIZE);
            if (bytes_read <= 0) {
                if (DEBUG_MODE) {
                    perror("read");
                }
                close(new_socket);
                exit(0);
            }
            
            // Parse command
            buffer[bytes_read] = '\0';
            if (DEBUG_MODE) {
                printf("Received command: %s\n", buffer);
            }
            
            if (strncmp(buffer, "DECRYPT ", 8) == 0) {
                // Command DECRYPT: Decrypt dan simpan file
                char* text_data = buffer + 8;
                char* result = save_decoded_file(text_data);
                
                // Log
                write_log("Client", "DECRYPT", "Text data");
                if (strncmp(result, "ERROR", 5) != 0) {
                    write_log("Server", "SAVE", result);
                }
                
                // Kirim hasil ke client
                send(new_socket, result, strlen(result), 0);
            } 
            else if (strncmp(buffer, "DOWNLOAD ", 9) == 0) {
                // Command DOWNLOAD: Kirim file ke client
                char* filename = buffer + 9;
                
                // Log
                write_log("Client", "DOWNLOAD", filename);
                
                // Ambil file
                size_t file_size;
                char* file_data = get_file(filename, &file_size);
                
                if (file_data) {
                    // Log
                    write_log("Server", "UPLOAD", filename);
                    
                    // Kirim ukuran file dulu
                    char size_str[32];
                    snprintf(size_str, sizeof(size_str), "%zu", file_size);
                    send(new_socket, size_str, strlen(size_str), 0);
                    
                    // Tunggu konfirmasi dari client
                    char ack[16];
                    read(new_socket, ack, sizeof(ack));
                    
                    // Kirim file data
                    send(new_socket, file_data, file_size, 0);
                    free(file_data);
                } else {
                    // File tidak ditemukan
                    const char* error_msg = "ERROR: File not found";
                    send(new_socket, error_msg, strlen(error_msg), 0);
                }
            }
            else if (strncmp(buffer, "EXIT", 4) == 0) {
                // Command EXIT: Log exit request
                write_log("Client", "EXIT", "Client requested to exit");
            }
            
            close(new_socket);
            exit(0);
        }
        
        close(new_socket);
    }
    
    return 0;
}

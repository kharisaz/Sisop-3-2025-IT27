# Sisop-3-2025-IT27

===================[Kelompok IT27]======================

Khumaidi Kharis Az-zacky - 5027241049

Mohamad Arkan Zahir Asyafiq - 5027241120

Abiyyu Raihan Putra Wikanto - 5027241042

============[Laporan Resmi Penjelasan Soal]=============

#soal_1
Pada soal ini, kita diminta untuk membuat sistem RPC (Remote Procedure Call) server-client yang dapat mengubah text file menjadi file JPEG. Server berjalan sebagai daemon di background dan client menyediakan menu interaktif untuk mengirim file text untuk didekripsi dan mengunduh file hasil.
Solusi yang diimplementasikan terdiri dari dua program utama:
-> image_server.c - Program server yang berjalan sebagai daemon
-> image_client.c - Program client dengan menu interaktif

A. Program Server (image_server.c)

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

Kode di atas berisi:

- Berbagai header yang diperlukan untuk operasi file, socket, dan sistem
- Definisi konstanta untuk port, ukuran buffer, dan panjang path maksimum
- Definisi mode DEBUG untuk memudahkan debugging (jika diset ke 1, server tidak berjalan sebagai daemon)

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
  
Fungsi ini:

- Menerima string input
- Membalikkan urutan karakter (karakter terakhir menjadi pertama, dst)
- Mengembalikan string hasil pembalikan
- Ini adalah langkah pertama dari proses dekripsi file

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
  
Kedua fungsi ini:

hex_to_int: Mengubah karakter hex ('0'-'9', 'a'-'f', 'A'-'F') menjadi nilai integer (0-15)
decode_hex: Mengubah string hex menjadi data binary, dengan setiap dua karakter hex menjadi satu byte data
Ini adalah langkah kedua dari proses dekripsi file

    void write_log(const char* source, const char* action, const char* info) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);
        
        // Buat direktori server jika belum ada
        mkdir("server", 0755);
        
        // Buat file log langsung di direktori server
        FILE* logfile = fopen("server/server.log", "a");
        if (!logfile) {
            if (DEBUG_MODE) {
                printf("Failed to open log file: %s\n", strerror(errno));
            }
            return;
        }
        
        fprintf(logfile, "[%s][%s]: [%s] [%s]\n", source, timestamp, action, info);
        fclose(logfile);
        
        if (DEBUG_MODE) {
            printf("[%s][%s]: [%s] [%s]\n", source, timestamp, action, info);
        }
    }
Fungsi ini:

Mendapatkan timestamp saat ini
Membuka file log di direktori server
Menulis log dengan format: [Source][YYYY-MM-DD hh:mm:ss]: [ACTION] [Info]
Dalam mode debug, juga mencetak log ke console

    char* save_decoded_file(const char* text_data) {
        time_t now = time(NULL);
        static char filename[64];
        snprintf(filename, sizeof(filename), "%ld.jpeg", now);
        
        // Step 1: Reverse text
        char* reversed = reverse_text(text_data);
        
        // Step 2: Decode dari hex ke binary
        size_t decoded_len;
        unsigned char* decoded = decode_hex(reversed, &decoded_len);
        
        if (!decoded) {
            free(reversed);
            return "ERROR: Failed to decode hex data";
        }
        
        // Pastikan direktori database ada
        mkdir("server", 0755);
        mkdir("server/database", 0755);
        
        // Buat path file output
        char filepath[PATH_MAX_LEN];
        snprintf(filepath, PATH_MAX_LEN, "server/database/%s", filename);
        
        // Buka file untuk menulis dalam mode binary
        FILE* outfile = fopen(filepath, "wb");
        if (!outfile) {
            free(reversed);
            free(decoded);
            return "ERROR: Failed to create output file";
        }
        
        // Tulis hasil decode ke file
        fwrite(decoded, 1, decoded_len, outfile);
        fclose(outfile);
        
        free(reversed);
        free(decoded);
        
        return filename;
    }

Fungsi ini:

Membuat nama file berdasarkan timestamp saat ini
Melakukan proses dekripsi: Reverse Text dan Decode Hex
Menyimpan hasil dekripsi sebagai file JPEG di direktori server/database/
Mengembalikan nama file yang disimpan atau pesan error jika gagal

    char* get_file(const char* filename, size_t* size) {
        // Buat path file
        char filepath[PATH_MAX_LEN];
        snprintf(filepath, PATH_MAX_LEN, "server/database/%s", filename);
        
        // Buka file
        FILE* file = fopen(filepath, "rb");
        if (!file) {
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

Fungsi ini:

Membuka file JPEG dari direktori server/database/
Membaca seluruh konten file ke dalam memory buffer
Mengembalikan buffer dan ukuran file, atau NULL jika file tidak ditemukan

    void daemonize() {
        if (DEBUG_MODE) {
            printf("Debug mode active - not daemonizing\n");
            return;
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
        
        // Tutup file descriptor standar
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        
        // Ignore certain signals
        signal(SIGCHLD, SIG_IGN);
        signal(SIGHUP, SIG_IGN);
    }

Fungsi ini:

Mengubah program menjadi daemon yang berjalan di background
Melakukan fork untuk memisahkan dari terminal
Membuat session ID baru
Menutup semua file descriptor standar
Mengabaikan beberapa signal yang tidak diperlukan

    int main() {
        int server_fd, new_socket;
        struct sockaddr_in address;
        int opt = 1;
        int addrlen = sizeof(address);
        char buffer[BUFFER_SIZE] = {0};
        
        // Buat direktori yang diperlukan
        mkdir("server", 0755);
        mkdir("server/database", 0755);
        
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
        
        // Jalankan sebagai daemon
        daemonize();
        
        // Loop untuk menerima koneksi
        while (1) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                continue;
            }
            
            // Fork untuk menangani klien baru
            pid_t pid = fork();
            
            if (pid < 0) {
                close(new_socket);
                continue;
            }
            
            if (pid == 0) { // Child process
                close(server_fd);
                
                // Terima command
                int bytes_read = read(new_socket, buffer, BUFFER_SIZE);
                if (bytes_read <= 0) {
                    close(new_socket);
                    exit(0);
                }
                
                // Parse command
                buffer[bytes_read] = '\0';
                
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

Fungsi utama server:

Menyiapkan socket untuk komunikasi
Menjalankan program sebagai daemon (jika tidak dalam mode debug)
Menjalankan loop tak terbatas untuk menerima koneksi dari client
Untuk setiap koneksi, melakukan fork untuk menangani client
Menerima dan memproses perintah dari client:

DECRYPT [text]: Dekripsi text dan simpan sebagai JPEG
DOWNLOAD [filename]: Kirim file JPEG ke client
EXIT: Log permintaan keluar dari client
Server tidak terminate saat terjadi error (error handling)

B. Program Client (image_client.c)

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

Kode ini berisi:

Header yang diperlukan untuk operasi file dan socket
Definisi konstanta untuk port, IP server, ukuran buffer, dan panjang path maksimum

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
Fungsi ini:

Membuat socket untuk komunikasi dengan server
Mengatur alamat server dan port
Mencoba melakukan koneksi ke server
Mengembalikan socket file descriptor jika berhasil, atau -1 jika gagal
Menampilkan pesan error yang sesuai jika terjadi kesalahan

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
Fungsi ini:

Membentuk path lengkap ke file input di client/secrets/
Memeriksa keberadaan file dan membuka file
Membaca seluruh konten file ke dalam memory
Membuat koneksi ke server
Mengirim perintah "DECRYPT" beserta data file ke server
Menerima dan memproses respons dari server
Menampilkan pesan sukses atau error sesuai respons server

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
    
Fungsi ini:

Membuat koneksi ke server
Mengirim perintah "DOWNLOAD" beserta nama file ke server
Menerima ukuran file dari server
Mengalokasikan buffer sesuai ukuran file
Menerima data file dari server
Menyimpan file ke direktori client/
Menampilkan pesan sukses atau error sesuai

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
            clear_screen();
        }
        
        return 0;
    }

Fungsi utama client:

Membuat direktori yang diperlukan (client/ dan client/secrets/)
Memeriksa ketersediaan file input
Memeriksa koneksi ke server
Menjalankan loop untuk menampilkan menu dan memproses pilihan user
Opsi menu:

Kirim file teks untuk didekripsi
Unduh file JPEG hasil dekripsi
Keluar


Setiap pilihan memanggil fungsi yang sesuai
Notifikasi ke server saat client keluar

#soal_2

Pada soal nomor 2, kami diminta untuk membuat Delivery Management System untuk perusahaan ekspedisi RushGo. Sistem ini menangani dua jenis pengiriman: Express (otomatis) dan Reguler (manual). Implementasi melibatkan dua program utama: dispatcher.c dan delivery_agent.c yang saling berkomunikasi menggunakan shared memory.

Dalam mengimplementasikan sistem ini, kami menggunakan beberapa konsep dan teknik pemrograman sistem operasi:
- Shared Memory: Digunakan untuk berbagi data pesanan antara dispatcher.c dan delivery_agent.c
- Multithreading: delivery_agent.c menggunakan thread untuk menjalankan 3 agen pengiriman secara paralel
- System Call: Digunakan untuk mengunduh file CSV dan berinteraksi dengan sistem operasi
- File I/O: Digunakan untuk operasi pembacaan CSV dan penulisan log

Penjelasan Kode Program
1. Struktur Data Utama
Berikut adalah struktur data yang digunakan dalam kedua program:
    
    
         // Struktur untuk menyimpan data pesanan
                typedef struct {
                    char nama[50];
                    char alamat[100];
                    char tipe[20]; // "Express" atau "Reguler"
                    bool terkirim;
                    char dikirimOleh[20]; // Nama agen yang mengirim
                } Pesanan;
                
                // Struktur untuk shared memory
                typedef struct {
                    int jumlahPesanan;
                    Pesanan pesanan[MAX_ORDERS];
                } SharedData;
       
Penjelasan:

Pesanan: Menyimpan detail setiap pesanan seperti nama penerima, alamat, tipe pengiriman, status pengiriman, dan nama agen pengirim
SharedData: Menyimpan array pesanan dan jumlah total pesanan untuk disimpan dalam shared memory

2. Program dispatcher.c
Inisialisasi Shared Memory

     // Buat atau dapatkan shared memory
            int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666 | IPC_CREAT);
            if (shmid == -1) {
                perror("shmget");
                exit(EXIT_FAILURE);
            }
            
            // Attach shared memory
            SharedData *sharedData = (SharedData *)shmat(shmid, NULL, 0);
            if (sharedData == (void *)-1) {
                perror("shmat");
                exit(EXIT_FAILURE);
            }

Penjelasan:

shmget(): Menciptakan atau mendapatkan segment shared memory dengan key dan ukuran tertentu
shmat(): Mengaitkan (attach) shared memory ke ruang alamat proses saat ini, mengembalikan pointer ke area shared memory

Pengunduhan dan Pemrosesan File CSV

    cint unduhCSV() {
            // Coba wget dulu
            char wget_cmd[512];
            sprintf(wget_cmd, "wget -q -O %s \"%s\"", CSV_FILE, CSV_URL);
            
            int wget_result = system(wget_cmd);
            if (wget_result == 0) {
                printf("Berhasil mengunduh file CSV menggunakan wget\n");
                return 1;
            }
            
            // Jika wget gagal, coba curl
            char curl_cmd[512];
            sprintf(curl_cmd, "curl -s -L \"%s\" -o %s", CSV_URL, CSV_FILE);
            
            int curl_result = system(curl_cmd);
            if (curl_result == 0) {
                printf("Berhasil mengunduh file CSV menggunakan curl\n");
                return 1;
            }
            
            printf("Gagal mengunduh file CSV. Coba buat file contoh...\n");
            return 0;
        }
Penjelasan:

Fungsi ini menggunakan system() untuk menjalankan perintah wget atau curl untuk mengunduh file CSV
Program mencoba wget terlebih dahulu, jika gagal maka mencoba curl
Jika kedua metode gagal, program akan membuat file CSV contoh

Memuat Data dari CSV ke Shared Memory

     cint muatDataDariCSV(SharedData *sharedData) {
            FILE *fp = fopen(CSV_FILE, "r");
            if (fp == NULL) {
                printf("File CSV tidak ditemukan. Mencoba unduh...\n");
                
                if (!unduhCSV()) {
                    buatContohCSV();
                }
                
                fp = fopen(CSV_FILE, "r");
                if (fp == NULL) {
                    printf("Error: Masih tidak bisa membuka file CSV\n");
                    return 0;
                }
            }
            
            char line[256];
            int count = 0;
            
            // Skip header
            fgets(line, sizeof(line), fp);
            
            while (fgets(line, sizeof(line), fp) && count < MAX_ORDERS) {
                char *token;
                
                // Ambil nama
                token = strtok(line, ",");
                if (token == NULL) continue;
                strncpy(sharedData->pesanan[count].nama, token, sizeof(sharedData->pesanan[count].nama) - 1);
                
                // Ambil alamat
                token = strtok(NULL, ",");
                if (token == NULL) continue;
                strncpy(sharedData->pesanan[count].alamat, token, sizeof(sharedData->pesanan[count].alamat) - 1);
                
                // Ambil tipe
                token = strtok(NULL, ",\n");
                if (token == NULL) continue;
                strncpy(sharedData->pesanan[count].tipe, token, sizeof(sharedData->pesanan[count].tipe) - 1);
                
                // Inisialisasi status pengiriman
                sharedData->pesanan[count].terkirim = false;
                strcpy(sharedData->pesanan[count].dikirimOleh, "");
                
                count++;
            }
            
            sharedData->jumlahPesanan = count;
            fclose(fp);
            
            printf("Berhasil memuat %d pesanan dari file CSV\n", count);
            return 1;
        }
Penjelasan:

Membuka file CSV dan membaca baris per baris
Melewati baris header
Memproses setiap baris CSV dengan strtok() untuk memisahkan nilai-nilai yang dipisahkan koma
Menginisialisasi status pengiriman dan pengirim untuk setiap pesanan
Menyimpan jumlah total pesanan di shared memory

Pengiriman Paket Reguler

     cvoid kirimPesananReguler(SharedData *sharedData, char *nama, char *namaUser) {
            for (int i = 0; i < sharedData->jumlahPesanan; i++) {
                if (strcmp(sharedData->pesanan[i].nama, nama) == 0 && 
                    strcmp(sharedData->pesanan[i].tipe, "Reguler") == 0 && 
                    !sharedData->pesanan[i].terkirim) {
                    
                    sharedData->pesanan[i].terkirim = true;
                    sprintf(sharedData->pesanan[i].dikirimOleh, "%s", namaUser);
                    
                    tambahLog(namaUser, nama, sharedData->pesanan[i].alamat, "Reguler");
                    
                    printf("Pesanan Reguler untuk %s berhasil dikirim oleh Agent %s\n", nama, namaUser);
                    return;
                }
            }
            
            printf("Error: Tidak menemukan pesanan Reguler dengan nama %s\n", nama);
        }
        
Penjelasan:

Mencari pesanan dengan nama tertentu dalam shared memory
Memeriksa apakah pesanan bertipe "Reguler" dan belum terkirim
Jika ditemukan, mengubah status pengiriman dan mencatat nama agent pengirim
Menambahkan log pengiriman ke file log
Jika tidak ditemukan, menampilkan pesan error

Command Line Interface
    // Periksa argumen command line
    if (argc > 1) {
        if (strcmp(argv[1], "-deliver") == 0 && argc == 3) {
            // Kirim pesanan Reguler
            char username[256];
            // Gunakan nama "Arkan" sebagai nama agen pengirim
            strcpy(username, "Arkan");
            kirimPesananReguler(sharedData, argv[2], username);
        } else if (strcmp(argv[1], "-status") == 0 && argc == 3) {
            // Cek status pesanan
            cekStatus(sharedData, argv[2]);
        } else if (strcmp(argv[1], "-list") == 0) {
            // Tampilkan semua pesanan
            tampilkanDaftarPesanan(sharedData);
        } else {
            printf("Penggunaan:\n");
            printf("  ./dispatcher -deliver [Nama] - untuk mengirim pesanan\n");
            printf("  ./dispatcher -status [Nama] - untuk memeriksa status pesanan\n");
            printf("  ./dispatcher -list - untuk melihat semua pesanan\n");
        }
    } else {
        // Inisialisasi - unduh dan muat data dari CSV
        printf("Memulai sistem RushGo...\n");
        printf("Mengunduh file pesanan...\n");
        
        // Muat data ke shared memory
        if (muatDataDariCSV(sharedData)) {
            printf("Berhasil memuat data ke shared memory\n");
        } else {
            printf("Gagal memuat data ke shared memory\n");
        }
    }
Penjelasan:

Memeriksa argumen command line untuk menentukan operasi yang dilakukan
-deliver [Nama]: Mengirim pesanan Reguler untuk nama tertentu
-status [Nama]: Mengecek status pesanan dengan nama tertentu
-list: Menampilkan daftar semua pesanan
Tanpa argumen: Menginisialisasi sistem dengan mengunduh dan memuat data CSV

3. Program delivery_agent.c
Inisialisasi dan Koneksi ke Shared Memory
    // Dapatkan shared memory
    int shmid = shmget(SHM_KEY, sizeof(SharedData), 0666);
    if (shmid == -1) {
        printf("Error: Shared memory belum dibuat. Jalankan dispatcher terlebih dahulu.\n");
        exit(EXIT_FAILURE);
    }
    
    // Attach shared memory
    SharedData *sharedData = (SharedData *)shmat(shmid, NULL, 0);
    if (sharedData == (void *)-1) {
        perror("shmat");
        exit(EXIT_FAILURE);
    }
Penjelasan:

Mendapatkan shared memory yang telah dibuat oleh dispatcher menggunakan key yang sama
Melakukan attach shared memory ke ruang alamat proses delivery_agent
Jika shared memory belum dibuat, program memberikan pesan error dan keluar

Implementasi Thread untuk Agen Pengiriman Express
    // Fungsi untuk thread agen
    void *agenExpress(void *arg) {
        DataAgen *dataAgen = (DataAgen *)arg;
        SharedData *sharedData = dataAgen->sharedData;
        
        // Set flag running ke true
        dataAgen->running = true;
        dataAgen->pesananDiproses = 0;
        
        printf("AGENT %s: Mulai mencari pesanan Express\n", dataAgen->nama);
        
        // Counter untuk membatasi jumlah percobaan
        int tryCount = 0;
        
        while (dataAgen->running && !berhenti && tryCount < MAX_TRY_COUNT) {
            bool found = false;
            
            // Cari pesanan Express yang belum dikirim
            pthread_mutex_lock(&mutex);
            for (int i = 0; i < sharedData->jumlahPesanan; i++) {
                if (strcmp(sharedData->pesanan[i].tipe, "Express") == 0 && 
                    !sharedData->pesanan[i].terkirim) {
                    
                    // Flag bahwa kita menemukan pesanan
                    found = true;
                    
                    // Kirim pesanan
                    sharedData->pesanan[i].terkirim = true;
                    strcpy(sharedData->pesanan[i].dikirimOleh, dataAgen->nama);
                    
                    // Simpan nama dan alamat untuk log
                    char nama[50], alamat[100];
                    strcpy(nama, sharedData->pesanan[i].nama);
                    strcpy(alamat, sharedData->pesanan[i].alamat);
                    
                    pthread_mutex_unlock(&mutex);
                    
                    // Tambahkan log
                    tambahLog(dataAgen->nama, nama, alamat, "Express");
                    
                    // Increment counter
                    dataAgen->pesananDiproses++;
                    
                    printf("AGENT %s: Pesanan Express untuk %s telah dikirim\n", 
                           dataAgen->nama, nama);
                    
                    // Reset counter karena kita menemukan pesanan
                    tryCount = 0;
                    
                    // Tunggu sejenak sebelum mencari pesanan berikutnya
                    sleep(1);
                    break;
                }
            }
            
            // Jika loop selesai tanpa melepaskan mutex
            if (!found) {
                pthread_mutex_unlock(&mutex);
            }
            
            // Jika tidak ada pesanan Express yang ditemukan
            if (!found) {
                tryCount++;
                
                // Tidur sebentar sebelum memeriksa kembali
                sleep(2);
            }
        }
        
        if (tryCount >= MAX_TRY_COUNT) {
            printf("AGENT %s: Selesai - total %d pesanan Express telah diproses\n", 
                   dataAgen->nama, dataAgen->pesananDiproses);
        }
        
        dataAgen->running = false;
        return NULL;
    }
Penjelasan:

Fungsi thread yang dieksekusi oleh setiap agen pengiriman Express
Menggunakan mutex untuk mengamankan akses ke shared memory
Mencari pesanan Express yang belum dikirim dalam shared memory
Jika ditemukan, menandai pesanan sebagai terkirim dan mencatat nama agen
Jika tidak ada pesanan ditemukan setelah beberapa kali percobaan, thread akan berhenti
Counter menampilkan jumlah pesanan yang telah diproses oleh setiap agen

Pembuatan Thread untuk Tiga Agen
    // Buat thread untuk tiga agen
    if (pthread_create(&dataAgenA.thread_id, NULL, agenExpress, &dataAgenA) != 0 ||
        pthread_create(&dataAgenB.thread_id, NULL, agenExpress, &dataAgenB) != 0 ||
        pthread_create(&dataAgenC.thread_id, NULL, agenExpress, &dataAgenC) != 0) {
        printf("Error: Gagal membuat thread\n");
        
        // Detach shared memory
        if (shmdt(sharedData) == -1) {
            perror("shmdt");
        }
        
        exit(EXIT_FAILURE);
    }
Penjelasan:

Membuat tiga thread terpisah untuk AGENT A, B, dan C
Setiap thread menjalankan fungsi agenExpress dengan parameter yang berbeda
Jika pembuatan thread gagal, program akan mendetach shared memory dan keluar

Auto-Exit Mechanism
    // Loop utama dengan auto-exit
    int checkCount = 0;
    bool autoExit = false;
    
    while (!berhenti) {
        // Tidur sejenak
        sleep(1);
        checkCount++;
        
        // Periksa apakah semua thread selesai
        if (!dataAgenA.running && !dataAgenB.running && !dataAgenC.running) {
            printf("Semua agen telah selesai mengirimkan pesanan Express\n");
            autoExit = true;
            break;
        }
        
        // Setiap CHECK_INTERVAL detik, periksa status pesanan
        if (checkCount >= CHECK_INTERVAL) {
            checkCount = 0;
            
            // Jika tidak ada lagi pesanan Express yang belum dikirim, persiapkan untuk keluar
            if (!masihAdaPesananExpress(sharedData)) {
                // Hitung total pesanan Express yang telah diproses
                int totalProcessed = dataAgenA.pesananDiproses + dataAgenB.pesananDiproses + 
                                    dataAgenC.pesananDiproses;
                
                printf("Semua pesanan Express telah terkirim (%d pesanan). Bersiap untuk berhenti...\n", 
                       totalProcessed);
                
                // Tandai untuk keluar dari loop utama
                autoExit = true;
                
                // Set flag untuk berhenti
                dataAgenA.running = false;
                dataAgenB.running = false;
                dataAgenC.running = false;
                break;
            }
        }
    }
Penjelasan:

Implementasi mekanisme auto-exit yang memungkinkan program berhenti secara otomatis
Secara berkala memeriksa apakah masih ada pesanan Express yang belum dikirim
Jika semua pesanan Express sudah terkirim, program akan berhenti secara otomatis
Program juga berhenti jika semua thread telah selesai

4. Logging System
    cvoid tambahLog(char *sumber, char *nama, char *alamat, char *tipe) {
        FILE *logFile = fopen(LOG_FILE, "a");
        if (logFile == NULL) {
            printf("Error: Tidak bisa membuka file log\n");
            return;
        }
        
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        
        fprintf(logFile, "[%02d/%02d/%04d %02d:%02d:%02d] [AGENT %s] %s package delivered to %s in %s\n",
                t->tm_mday, t->tm_mon + 1, t->tm_year + 1900,
                t->tm_hour, t->tm_min, t->tm_sec,
                sumber, tipe, nama, alamat);
        
        fclose(logFile);
    }
Penjelasan:

Mencatat setiap pengiriman ke file log dengan format yang telah ditentukan
Menggunakan time() dan localtime() untuk mendapatkan timestamp saat ini
Format log: [dd/mm/yyyy hh:mm:ss] [AGENT X] Type package delivered to [Nama] in [Alamat]
#soal_3
Untuk soal ini, kami diminta untuk membuat server dengan gim yang terdiri dari dungeon.c, player.c, shop.h/shop.c.

shop.h

    #ifndef SHOP_H
    #define SHOP_H

Header guard yang digunakan agar file ini tidak dimasukkan lebih dari sekali saat proses kompilasi.

#ifndef artinya jika SHOP_H belum didefinisikan

#define SHOP_H mendefinisikan SHOP_H, sehingga jika nanti file ini di-include lagi, isinya akan diabaikan (menghindari duplikasi simbol).

    #define MAX_WEAPONS 6
    #define NAME_LEN    32
    #define PASS_LEN    64

MAX_WEAPONS: jumlah maksimal senjata di toko.

NAME_LEN: panjang maksimal nama senjata.

PASS_LEN: panjang maksimal deskripsi passive senjata.

    typedef struct {
        int  id;
        char name[NAME_LEN];
        int  price;
        int  damage;
        int  hasPassive;           // 0 = no, 1 = yes
        char passiveDesc[PASS_LEN];
    } Weapon;

Struktur ini mewakili sebuah senjata dan terdiri dari:

id: ID unik senjata.

name: nama senjata (maks 32 karakter).

price: harga senjata.

damage: jumlah damage yang bisa dihasilkan.

hasPassive: apakah senjata punya efek tambahan (1) atau tidak (0).

passiveDesc: deskripsi dari efek pasif (jika ada).

    void getShopList(Weapon **list, int *count);

Ini adalah deklarasi fungsi getShopList. Fungsi ini memberikan daftar senjata yang tersedia di toko. Array yang dikembalikan adalah static, artinya sudah ada di memori, dan tidak boleh di-free oleh pemanggil fungsi.

Weapon **list: pointer ke pointer yang nanti akan menunjuk ke array senjata.

int *count: untuk mengembalikan jumlah senjata.

    #endif // SHOP_H

Akhir dari header guard

shop.c

    #include <string.h>
    #include "shop.h"

Memasukkan deklarasi yang dari shop.h sehingga definisi Weapon dan getShopList() dapat digunakan

    static Weapon weapons[MAX_WEAPONS] = {
        { 0, "Iron Sword",    20,  8, 0, "" },
        { 1, "Steel Axe",     30, 10, 1, "+10% Crit Chance" },
        { 2, "Magic Wand",    50, 12, 1, "Chance to Silence" },
        { 3, "Wooden Club",   10,  5, 0, "" },
        { 4, "Dragon Spear",  80, 15, 0, "" },
        { 5, "Shadow Dagger", 35,  9, 1, "+20% Attack Speed" }
    };

Array statis dari objek Weapon

    void getShopList(Weapon **list, int *count) {
        *list  = weapons;
        *count = MAX_WEAPONS;
    }

Fungsi getShopList yang digunakan untuk memberikan daftar Weapon

dungeon.c

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <arpa/inet.h>

Digunakan untuk input/output, string, socket, dan alamat IP.

    #define SERVER_IP   "127.0.0.1"
    #define SERVER_PORT 8080
    #define BUF_SIZE    512

Alamat server lokal (localhost) dengan port 8080.

Buffer untuk menerima data dari server (maks 512 byte).

    void showMenu() {
        puts("\n== WELCOME TO DUNGEON ==");
        puts("1. Show Player Stats");
        puts("2. View Inventory & Equip Weapons");
        puts("3. Shop");
        puts("4. Battle Mode");
        puts("5. Exit");
        printf("Choose an option: ");
    }

Menampilkan menu pilihan ke pemain.

    void readFullResponse(int sock) {
        char buf[BUF_SIZE];
        while (1) {
            int n = read(sock, buf, BUF_SIZE - 1);
            if (n <= 0) break;
            buf[n] = 0;
            printf("%s", buf);
            if (n < BUF_SIZE - 1) break;
        }
    }

Menerima pesan dari server lalu menulisnya di terminal. Jika pesannya kurang dari buffer maksimal, maka dianggap selesai.

    int main() {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in serv = {
            .sin_family = AF_INET,
            .sin_port   = htons(SERVER_PORT)
        };
        inet_pton(AF_INET, SERVER_IP, &serv.sin_addr);

Membuat socket TCP.

Menyiapkan alamat server (AF_INET, port 8080, IP 127.0.0.1).

      if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
          perror("connect");
          return 1;
      }
      printf("[CLIENT] Connected to %s:%d\n", SERVER_IP, SERVER_PORT);

Mencoba menyambung ke server. Jika gagal, maka menampilkan error dan keluar. Jika berhasil, maka menulis hasilnya.

      char choice[64], cmd[64];
      while (1) {
          showMenu();
          if (!fgets(choice, sizeof(choice), stdin)) break;
          int opt = atoi(choice);
  
          switch (opt) {
              case 1:
                  strcpy(cmd, "SHOW_STATS\n");
                  write(sock, cmd, strlen(cmd));
                  readFullResponse(sock);
                  break;

Switchcase untuk menunya. Case 1 digunakan untuk menunjukkan statistik player dengan mengirimkan SHOW_STATS ke server dan menulis balasannya.

              case 2:
                  strcpy(cmd, "INVENTORY\n");
                  write(sock, cmd, strlen(cmd));
                  readFullResponse(sock);
  
                  while (1) {
                      printf("Enter EQUIP <index> or BACK: ");
                      if (!fgets(choice, sizeof(choice), stdin)) break;
                      if (strncmp(choice, "EQUIP", 5) == 0) {
                          write(sock, choice, strlen(choice));
                          readFullResponse(sock);
                      } 
                      else if (strncmp(choice, "BACK", 4) == 0) {
                          break;
                      } 
                      else {
                          printf("Invalid. Use EQUIP <index> or BACK.\n");
                      }
                  }
                  break;

Case 2 untuk menunjukkan inventory milik player. Juga menunjukkan tulisan Enter EQUIP <index or BACK: sebagai instruksi ke player.

              case 3:
                  strcpy(cmd, "SHOP\n");
                  write(sock, cmd, strlen(cmd));
                  readFullResponse(sock);
  
                  while (1) {
                      printf("Enter BUY <id> or BACK: ");
                      if (!fgets(choice, sizeof(choice), stdin)) break;
                      if (strncmp(choice, "BUY", 3) == 0) {
                          write(sock, choice, strlen(choice));
                          readFullResponse(sock);
                      } 
                      else if (strncmp(choice, "BACK", 4) == 0) {
                          break;
                      } 
                      else {
                          printf("Invalid. Use BUY <id> or BACK.\n");
                      }
                  }
                  break;

Menampilkan daftar senjata dari shop. Menuliskan Enter BUY <id> or BACK sebagai instruksi untuk player.

              case 4:
                  strcpy(cmd, "BATTLE\n");
                  write(sock, cmd, strlen(cmd));
                  readFullResponse(sock);
                  while (1) {
                      printf("(battle) enter ATTACK or EXIT_BATTLE: ");
                      if (!fgets(choice, sizeof(choice), stdin)) break;
                      if (strncmp(choice, "ATTACK", 6) == 0)
                          strcpy(cmd, "ATTACK\n");
                      else if (strncmp(choice, "EXIT", 4) == 0)
                          strcpy(cmd, "EXIT_BATTLE\n");
                      else {
                          puts("Invalid. Use ATTACK or EXIT_BATTLE.");
                          continue;
                      }
                      write(sock, cmd, strlen(cmd));
                      readFullResponse(sock);
                      if (strncmp(cmd, "EXIT_BATTLE", 11) == 0) break;
                  }
                  break;

Memasuki mode Battle. Player bisa menulis ATTACK untuk menyerang atau EXIT_BATTLE untuk keluar.

              case 5:
                  strcpy(cmd, "EXIT\n");
                  write(sock, cmd, strlen(cmd));
                  close(sock);
                  return 0;
  
              default:
                  puts("Invalid option. Try again.");
                  break;
          }
      }
  
      close(sock);
      return 0;
      }

Keluar dari gim dengan mengirim perintah EXIT ke server, menutup konseksi, lalu mengakhiri program.

Player.c

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <pthread.h>
    #include <time.h>
    #include <arpa/inet.h>
    #include "shop.c"

stdio.h, stdlib.h, string.h, unistd.h, time.h: library dasar C untuk I/O, memori, string, fungsi sleep dan waktu.

pthread.h: digunakan untuk multithreading (agar server bisa melayani banyak klien secara paralel).

arpa/inet.h: digunakan untuk fungsi socket seperti htons, inet_ntoa, dll.

shop.c: file eksternal yang berisi daftar senjata (Weapon) dan fungsi getShopList
  
    #define PORT         8080
    #define MAX_CLIENTS  10
    #define BUF_SIZE     512
    #define MAX_INV      20
    #define MAX_NAME     32

Makro untuk mengatur batasan program:

PORT: port TCP server.

MAX_CLIENTS: maksimal jumlah klien yang bisa dilayani.

BUF_SIZE: ukuran buffer untuk pesan teks.

MAX_INV: maksimal senjata di inventory pemain.

MAX_NAME: panjang nama maksimal

    int      gold[MAX_CLIENTS];
    int      invCount[MAX_CLIENTS];
    Weapon   inventory[MAX_CLIENTS][MAX_INV];
    int      equippedId[MAX_CLIENTS];    
    int      kills[MAX_CLIENTS];
    
    int      inBattle[MAX_CLIENTS];
    int      enemyHp[MAX_CLIENTS];
    int      enemyMaxHp[MAX_CLIENTS];
    int      enemyReward[MAX_CLIENTS];

Setiap array ini menyimpan data per pemain (indeks berdasarkan cid / client ID):

gold: jumlah emas pemain.

invCount: jumlah item di inventory pemain.

inventory: array senjata untuk setiap pemain.

equippedId: ID dari senjata yang sedang dipakai.

kills: jumlah musuh yang dikalahkan.

Untuk pertarungan:

inBattle: status apakah pemain sedang bertarung.

enemyHp, enemyMaxHp: HP musuh saat ini dan maksimal.

enemyReward: emas yang didapat jika musuh dikalahkan.

    const Weapon FISTS = { -1, "Fists", 0, 5, 0, "" };

Senjata default jika pemain belum membeli/menyimpan senjata. Damage 5, tanpa harga, tanpa pasif.

    static int randRange(int a, int b) {
        return a + rand() % (b - a + 1);
    }
    
    void spawnEnemy(int cid) {
        enemyMaxHp[cid]  = randRange(50, 200);
        enemyHp[cid]     = enemyMaxHp[cid];
        enemyReward[cid] = randRange(10, 50);
        inBattle[cid]    = 1;
    }

Digunakan saat pemain memulai pertarungan. Musuh diberikan HP dan reward emas acak.

    void buildHpBar(int cid, char *out) {
        int total = 20;
        int filled = (int)((double)enemyHp[cid] / enemyMaxHp[cid] * total);
        if (filled<0) filled=0;
        char bar[32] = {0};
        int i;
        for (i=0;i<filled;i++)    bar[i] = '=';
        for (; i<total; i++)      bar[i] = ' ';
        sprintf(out, "[%s] HP: %d/%d\n", bar, enemyHp[cid], enemyMaxHp[cid]);
    }

Membuat string seperti [===== ] HP: 50/100 untuk menunjukkan status musuh. Digunakan saat menyerang musuh.

    void handle_client(int clientSock, int cid) {
        char buf[BUF_SIZE], resp[BUF_SIZE];
        Weapon *shopList; int shopCount;
        getShopList(&shopList, &shopCount);

Signature & Parameter

clientSock: socket descriptor untuk komunikasi dengan klien ini.

cid: client ID (0…MAX_CLIENTS–1) yang dipakai untuk mengindeks state pemain.

Buffer

buf[BUF_SIZE] untuk menampung data masuk dari client.

resp[BUF_SIZE] untuk menyusun pesan balasan.

Daftar Shop

Weapon *shopList; int shopCount; diisi dengan getShopList() dari modul shop (shop.c).

shopList menunjuk ke array senjata statis, shopCount berisi jumlahnya.

      gold[cid]        = 100;
      invCount[cid]    = 0;
      equippedId[cid]  = -1;
      kills[cid]       = 0;
      inBattle[cid]    = 0;

Inisialisasi State Pemain

Gold mulai di-setup jadi 100.

invCount (jumlah item di inventory) = 0.

equippedId = –1 menandakan belum ada senjata terpasang.

kills = 0.

inBattle = 0 (belum bertarung).

      while (1) {
          memset(buf, 0, BUF_SIZE);
          int len = read(clientSock, buf, BUF_SIZE-1);
          if (len<=0) break;
          buf[strcspn(buf, "\r\n")] = 0;

memset bersihkan buf.

read(...) membaca sampai BUF_SIZE-1 byte dari socket.

Jika len ≤ 0, artinya klien memutus koneksi → keluar loop.

buf[strcspn(buf, "\r\n")] = 0; buang \r atau \n di akhir input.

          char cmd[16]; int arg;
          arg = -1;
          sscanf(buf, "%15s %d", cmd, &arg);

Parsing Perintah

cmd menampung token pertama (maks 15 karakter).

arg menampung angka setelah perintah (jika ada), default -1.

Contoh: BUY 2 → cmd = "BUY", arg = 2.

          if (strcmp(cmd,"SHOW_STATS")==0) {
              Weapon eq = (equippedId[cid] < 0
                           ? FISTS
                           : inventory[cid][equippedId[cid]]);
            
Mengecek apakah pemain sudah memilih senjata (equippedId[cid]).

Jika belum (< 0), gunakan senjata default FISTS.

Kalau sudah, ambil dari inventory[cid].
            
              int n = snprintf(resp, BUF_SIZE,
                  "Gold: %dG\n"
                  "Weapon: %s\n"
                  "Base Damage: %d\n"
                  "%s"
                  "Kills: %d\n",
                  gold[cid],
                  eq.name,
                  eq.damage,
                  (eq.hasPassive
                    ? (char[]){0} , sprintf(resp+strlen(resp),"Passive: %s\n",eq.passiveDesc), resp
                    : ""),
                  kills[cid]
              );
              write(clientSock, resp, strlen(resp));
          }

Menyusun resp berisi status pemain: jumlah gold, senjata aktif, damage, deskripsi passive (jika ada), dan total kill.

hasPassive: dipakai agar bisa menyisipkan deskripsi passive jika senjata punya efek passive.
Hasil dikirim ke klien via write.
        
          else if (strcmp(cmd,"SHOP")==0) {
              int i; 
              strcpy(resp,"=== WEAPON SHOP ===\n");
              for (i=0;i<shopCount;i++) {
                  char line[128];
                  sprintf(line, "%d. %s - %dG - Dmg %d%s\n",
                      shopList[i].id,
                      shopList[i].name,
                      shopList[i].price,
                      shopList[i].damage,
                      (shopList[i].hasPassive
                         ? " (Passive)"
                         : "")
                  );
                  strcat(resp,line);
              }
              write(clientSock, resp, strlen(resp));
          }

Menampilkan daftar senjata di toko (shopList).

Tiap senjata: ID, nama, harga, damage, dan indikator passive jika ada.

Semua info dijadikan string dan dikirim ke klien.
        
          else if (strcmp(cmd,"BUY")==0) {
              int found=-1;
              for (int i=0;i<shopCount;i++)
                  if (shopList[i].id==arg) { found=i; break; }

Cari senjata yang ingin dibeli (arg adalah ID-nya).

Kalau tidak ditemukan, found tetap –1.
                
              if (found<0) {
                  strcpy(resp,"Invalid weapon ID.\n");
              } else if (gold[cid] < shopList[found].price) {
                  strcpy(resp,"Not enough gold!\n");
              } else if (invCount[cid] >= MAX_INV) {
                  strcpy(resp,"Inventory full!\n");
              } else {
                  gold[cid] -= shopList[found].price;
                  inventory[cid][invCount[cid]++] = shopList[found];
                  sprintf(resp,"You bought: %s\n", shopList[found].name);
              }
              write(clientSock, resp, strlen(resp));
          }

Cek validasi:

ID tidak valid → error.

Gold kurang → error.

Inventory penuh → error.

Kalau semua valid, senjata ditambahkan ke inventory[cid], gold dikurangi, dan invCount bertambah.

Kirim hasil pembelian ke klien.
        
          else if (strcmp(cmd,"INVENTORY")==0) {
              if (invCount[cid]==0) {
                  strcpy(resp,"Your inventory is empty.\n");
              } else {
                  strcpy(resp,"=== YOUR INVENTORY ===\n");
                  for (int i=0;i<invCount[cid];i++) {
                      char line[128];
                      sprintf(line, "%d. %s - Dmg %d%s\n",
                          i,
                          inventory[cid][i].name,
                          inventory[cid][i].damage,
                          (inventory[cid][i].hasPassive ? " (Passive)" : "")
                      );
                      strcat(resp,line);
                  }
                  strcat(resp,"Use: EQUIP <index>\n");
              }
              write(clientSock, resp, strlen(resp));
          }

Jika inventory kosong → tampilkan pesan khusus.

Jika tidak, tampilkan semua item:

Index (urutan)

Nama senjata

Damage

Keterangan (Passive) jika punya efek tambahan

Tambahkan instruksi untuk menggunakan EQUIP <index>.
        
          else if (strcmp(cmd,"EQUIP")==0) {
              if (arg<0 || arg>=invCount[cid]) {
                  strcpy(resp,"Invalid inventory index.\n");
              } else {
                  equippedId[cid] = arg;
                  sprintf(resp,"Equipped: %s\n",
                      inventory[cid][arg].name);
              }
              write(clientSock, resp, strlen(resp));
          }

Cek apakah index valid (arg adalah index senjata dalam inventory).

Kalau valid → atur equippedId[cid] agar menunjuk ke senjata itu.

Kirim konfirmasi ke klien.
        
          else if (strcmp(cmd,"BATTLE")==0) {
              spawnEnemy(cid);
              strcpy(resp,"Enemy appeared!\n");
              buildHpBar(cid, resp+strlen(resp));
              write(clientSock, resp, strlen(resp));
          }

Panggil spawnEnemy() untuk menghasilkan musuh baru.

Tampilkan pesan musuh muncul + HP bar musuh dengan buildHpBar().

Kirim ke klien.
        
          else if (strcmp(cmd,"ATTACK")==0) {
              if (!inBattle[cid]) {
                  strcpy(resp,"You are not in battle. Use BATTLE first.\n");
                  write(clientSock, resp, strlen(resp));
              } else 

Kalau pemain belum masuk battle, beri peringatan.
            
              {
                  Weapon eq = (equippedId[cid]<0?FISTS:inventory[cid][equippedId[cid]]);
                  int base = eq.damage;
                  int bonus = randRange(0, base);
                  int dmg   = base + bonus;
                  int isCrit = (randRange(1,100)<=10);
                  if (isCrit) dmg*=2;
                  int passiveOn = (eq.hasPassive && randRange(1,100)<=20);

Tentukan senjata yang dipakai.

Hitung damage:

Bonus acak dari 0–base.

Peluang kritikal 10% → damage x2.

Jika senjata punya passive, 20% peluang efek aktif.

                  enemyHp[cid] -= dmg;
                  int justKilled = (enemyHp[cid] <= 0);
                  if (justKilled) {
                      gold[cid] += enemyReward[cid];
                      kills[cid]++;
                  }

Kurangi HP musuh.

Kalau musuh mati, tambahkan gold dan kill count.

                  char line[BUF_SIZE];
                  int pos=0;
                  pos+=sprintf(line+pos,"You dealt %d damage%s\n",
                               dmg, isCrit?" (Critical!)":"");
                  if (passiveOn)
                      pos+=sprintf(line+pos,"[Passive: %s]\n",eq.passiveDesc);
                  if (justKilled) {
                      pos+=sprintf(line+pos,
                          "Enemy defeated! You gained %dG\n",
                          enemyReward[cid]);
                      spawnEnemy(cid);
                      pos+=sprintf(line+pos,"New enemy appears!\n");
                  }
                  buildHpBar(cid, line+pos);
                  write(clientSock, line, strlen(line));
              }
          }

Buat pesan hasil serangan: jumlah damage, apakah critical, efek passive, hasil kill.

Jika musuh mati, panggil spawnEnemy() lagi → muncul musuh baru.

Tambahkan HP bar musuh baru.

Kirim ke klien.
        
          else if (strcmp(cmd,"EXIT_BATTLE")==0) {
              inBattle[cid]=0;
              strcpy(resp,"Exited battle mode.\n");
              write(clientSock, resp, strlen(resp));
          }

Menyatakan bahwa pemain ingin kabur dari pertempuran.

Set inBattle[cid] = 0 agar status keluar dari battle.

Kirim pesan "Exited Battle mode." ke klien
        
          else if (strcmp(cmd,"EXIT")==0) {
              break;
          }

Keluar dari loop while (1).
    
          else {
              strcpy(resp,"Unknown command.\n");
              write(clientSock, resp, strlen(resp));
          }
      }

Jika ada command yang tidak dikenal, maka dibalas dengan pesan "Unknown command."

      close(clientSock);
      printf("[Client %d] Disconnected.\n", cid);
  }

Keluar dari loop, socket klien ditutup menggunakan close() untuk membebaskan sumber daya dan menulis Client Disconnected.

    // Struct untuk argumen thread
    typedef struct {
        int sock;
        int cid;
    } ClientArgs;

sock
Menyimpan descriptor socket (clientSock) yang akan dipakai untuk komunikasi dengan satu klien.

cid
Client ID (0…MAX_CLIENTS–1) yang menjadi indeks untuk state pemain (gold, inventory, dsb.).

Struct ini dibutuhkan agar kita bisa “mengemas” dua parameter (sock dan cid) menjadi satu pointer saat membuat thread.

  void *client_thread(void *arg) {
      ClientArgs *args = (ClientArgs *)arg;
      handle_client(args->sock, args->cid);
      free(args);
      return NULL;
  }

Cast
Pointer arg diubah (cast) kembali menjadi ClientArgs *.

Panggil
Memanggil handle_client(sock, cid) dengan data yang sudah dikemas.

Free
Setelah handle_client selesai (klien disconnect), memori args (yang dialokasikan di main) dibebaskan agar tidak terjadi memory leak.

Return
NULL karena fungsi thread tidak perlu mengembalikan data apapun.

    //Server
    int main() {
        srand((unsigned)time(NULL));

Seed RNG

Untuk memastikan setiap run rand() menghasilkan urutan berbeda.

      int serverFd = socket(AF_INET, SOCK_STREAM, 0);
      struct sockaddr_in addr = {
          .sin_family = AF_INET,
          .sin_port   = htons(PORT),
          .sin_addr.s_addr = INADDR_ANY
      };
      bind(serverFd, (struct sockaddr*)&addr, sizeof(addr));
      listen(serverFd, MAX_CLIENTS);
      printf("[SERVER] Listening on port %d...\n", PORT);

socket(): Buat TCP socket.

bind(): Kaitkan socket ke PORT pada semua interface (INADDR_ANY).

listen(): Siapkan antrian koneksi hingga MAX_CLIENTS.

Cetak status bahwa server sudah berjalan.

      for (int cid=0; cid<MAX_CLIENTS; cid++) {
          invCount[cid]=0;
          inBattle[cid]=0;
      }

Inisialisasi state awal untuk semua slot klien: inventory kosong dan bukan di battle.

      int clientSock;
      struct sockaddr_in cliAddr;
      socklen_t cliLen = sizeof(cliAddr);
      int cid = 0;
      while ((clientSock = accept(serverFd,
                  (struct sockaddr*)&cliAddr, &cliLen)) >= 0)
      {
          printf("[Client %d] Connected.\n", cid);
          pthread_t th;
          ClientArgs *args = malloc(sizeof(ClientArgs));
          args->sock = clientSock;
          args->cid  = cid;
          pthread_create(&th, NULL, client_thread, args);
          pthread_detach(th);
          if (++cid >= MAX_CLIENTS) break;
      }

accept(): Tunggu dan terima koneksi masuk → dapatkan clientSock.

Log: Cetak Client <cid> Connected.

Alloc & Isi ClientArgs:

malloc untuk menyimpan sock dan cid.

pthread_create: Buat thread baru yang menjalankan client_thread(args).

pthread_detach: Melepaskan thread sehingga hasilnya otomatis dibersihkan saat selesai, tanpa perlu pthread_join.

Increment cid: Siapkan slot berikutnya; jika sudah mencapai MAX_CLIENTS, break loop (tidak menerima lagi).

      close(serverFd);
      return 0;
  }

Setelah loop selesai (bila slot penuh atau error), socket server ditutup dan program berakhir.

#soal_4

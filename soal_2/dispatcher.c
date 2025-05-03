#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdbool.h>

#define SHM_KEY 1234
#define MAX_ORDERS 100
#define CSV_FILE "delivery_order.csv"
#define LOG_FILE "delivery.log"
#define CSV_URL "https://drive.usercontent.google.com/u/0/uc?id=1OJfRuLgsBnIBWtdRXbRsD2sG6NhMKOg9&export=download"

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

// Fungsi untuk menambahkan log
void tambahLog(char *sumber, char *nama, char *alamat, char *tipe) {
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

// Fungsi untuk mengunduh file CSV menggunakan wget atau curl
int unduhCSV() {
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

// Fungsi untuk membuat contoh file CSV jika download gagal
void buatContohCSV() {
    FILE *fp = fopen(CSV_FILE, "w");
    if (fp == NULL) {
        printf("Error: Tidak bisa membuat file CSV\n");
        return;
    }
    
    // Header
    fprintf(fp, "Nama,Alamat,Tipe\n");
    
    // Contoh data
    fprintf(fp, "John Doe,Jl. Contoh No. 1,Express\n");
    fprintf(fp, "Jane Smith,Jl. Contoh No. 2,Express\n");
    fprintf(fp, "Bob Johnson,Jl. Contoh No. 3,Reguler\n");
    fprintf(fp, "Alice Williams,Jl. Contoh No. 4,Reguler\n");
    fprintf(fp, "Charlie Brown,Jl. Contoh No. 5,Express\n");
    fprintf(fp, "Diana Prince,Jl. Contoh No. 6,Reguler\n");
    fprintf(fp, "Evan Peters,Jl. Contoh No. 7,Express\n");
    fprintf(fp, "Fiona Gallagher,Jl. Contoh No. 8,Reguler\n");
    
    fclose(fp);
    printf("File CSV contoh telah dibuat: %s\n", CSV_FILE);
}

// Fungsi untuk memuat data dari CSV ke shared memory
int muatDataDariCSV(SharedData *sharedData) {
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

// Fungsi untuk mengirim pesanan Reguler
void kirimPesananReguler(SharedData *sharedData, char *nama, char *namaUser) {
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

// Fungsi untuk memeriksa status pesanan
void cekStatus(SharedData *sharedData, char *nama) {
    for (int i = 0; i < sharedData->jumlahPesanan; i++) {
        if (strcmp(sharedData->pesanan[i].nama, nama) == 0) {
            if (sharedData->pesanan[i].terkirim) {
                printf("Status for %s: Delivered by Agent %s\n", nama, sharedData->pesanan[i].dikirimOleh);
            } else {
                printf("Status for %s: Pending\n", nama);
            }
            return;
        }
    }
    
    printf("Error: Tidak menemukan pesanan dengan nama %s\n", nama);
}

// Fungsi untuk menampilkan semua pesanan
void tampilkanDaftarPesanan(SharedData *sharedData) {
    printf("=== DAFTAR PESANAN ===\n");
    printf("%-20s %-15s %-10s\n", "Nama", "Tipe", "Status");
    printf("-------------------------------------------\n");
    
    for (int i = 0; i < sharedData->jumlahPesanan; i++) {
        printf("%-20s %-15s %-10s", 
               sharedData->pesanan[i].nama, 
               sharedData->pesanan[i].tipe,
               sharedData->pesanan[i].terkirim ? "Delivered" : "Pending");
        
        if (sharedData->pesanan[i].terkirim) {
            printf(" (by Agent %s)", sharedData->pesanan[i].dikirimOleh);
        }
        
        printf("\n");
    }
}

// Fungsi utama
int main(int argc, char *argv[]) {
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
    
    // Periksa argumen command line
    if (argc > 1) {
        if (strcmp(argv[1], "-deliver") == 0 && argc == 3) {
            // Kirim pesanan Reguler
            char username[256];
            // Jika getlogin_r() gagal, gunakan nama default
            if (getlogin_r(username, sizeof(username)) != 0) {
                strcpy(username, "user");
            }
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
    
    // Detach shared memory
    if (shmdt(sharedData) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
    
    return 0;
}

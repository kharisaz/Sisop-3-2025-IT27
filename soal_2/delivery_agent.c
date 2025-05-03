#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <signal.h>
#include <stdbool.h>

#define SHM_KEY 1234
#define MAX_ORDERS 100
#define LOG_FILE "delivery.log"
#define MAX_TRY_COUNT 30 // Batas percobaan (60 detik)

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

// Struktur untuk data thread agen
typedef struct {
    char nama[10];
    SharedData *sharedData;
    pthread_t thread_id;
    bool running;
} DataAgen;

// Data agen global
DataAgen dataAgenA, dataAgenB, dataAgenC;

// Flag untuk menandakan program harus berhenti
volatile sig_atomic_t berhenti = 0;

// Handler untuk SIGINT (Ctrl+C)
void handle_sigint(int sig) {
    printf("\nMenerima sinyal interrupt. Menghentikan agen...\n");
    berhenti = 1;
    
    // Set flag running ke false untuk semua agen
    dataAgenA.running = false;
    dataAgenB.running = false;
    dataAgenC.running = false;
}

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

// Helper function untuk memeriksa apakah masih ada pesanan Express yang belum terkirim
bool masihAdaPesananExpress(SharedData *sharedData) {
    for (int i = 0; i < sharedData->jumlahPesanan; i++) {
        if (strcmp(sharedData->pesanan[i].tipe, "Express") == 0 && !sharedData->pesanan[i].terkirim) {
            return true;
        }
    }
    return false;
}

// Fungsi untuk thread agen
void *agenExpress(void *arg) {
    DataAgen *dataAgen = (DataAgen *)arg;
    SharedData *sharedData = dataAgen->sharedData;
    
    // Set flag running ke true
    dataAgen->running = true;
    
    printf("AGENT %s: Mulai mencari pesanan Express\n", dataAgen->nama);
    
    // Counter untuk membatasi jumlah percobaan
    int tryCount = 0;
    int expressCount = 0;
    
    while (dataAgen->running && !berhenti && tryCount < MAX_TRY_COUNT) {
        bool found = false;
        
        // Cari pesanan Express yang belum dikirim
        for (int i = 0; i < sharedData->jumlahPesanan; i++) {
            if (strcmp(sharedData->pesanan[i].tipe, "Express") == 0 && 
                !sharedData->pesanan[i].terkirim) {
                
                // Flag bahwa kita menemukan pesanan
                found = true;
                expressCount++;
                
                // Kirim pesanan
                sharedData->pesanan[i].terkirim = true;
                strcpy(sharedData->pesanan[i].dikirimOleh, dataAgen->nama);
                
                // Tambahkan log
                tambahLog(dataAgen->nama, 
                         sharedData->pesanan[i].nama, 
                         sharedData->pesanan[i].alamat, 
                         "Express");
                
                printf("AGENT %s: Pesanan Express untuk %s telah dikirim\n", 
                       dataAgen->nama, 
                       sharedData->pesanan[i].nama);
                
                // Reset counter karena kita menemukan pesanan
                tryCount = 0;
                
                // Tunggu sejenak sebelum mencari pesanan berikutnya
                sleep(1);
                break;
            }
        }
        
        // Jika tidak ada pesanan Express yang ditemukan
        if (!found) {
            tryCount++;
            
            // Jika sudah mencapai setengah dari MAX_TRY_COUNT, berikan informasi
            if (tryCount == MAX_TRY_COUNT / 2) {
                printf("AGENT %s: Tidak menemukan pesanan Express lain, akan terus mencoba beberapa saat...\n", 
                       dataAgen->nama);
            }
            
            // Tidur sebentar sebelum memeriksa kembali
            sleep(2);
        }
    }
    
    if (tryCount >= MAX_TRY_COUNT) {
        printf("AGENT %s: Selesai - total %d pesanan Express telah diproses\n", 
               dataAgen->nama, expressCount);
    } else if (berhenti) {
        printf("AGENT %s: Berhenti karena sinyal interrupt\n", dataAgen->nama);
    } else {
        printf("AGENT %s: Berhenti karena flag running dimatikan\n", dataAgen->nama);
    }
    
    dataAgen->running = false;
    return NULL;
}

int main() {
    // Set up signal handler
    signal(SIGINT, handle_sigint);
    
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
    
    printf("Memulai agen pengiriman Express RushGo...\n");
    
    // Inisialisasi data untuk tiga agen
    strcpy(dataAgenA.nama, "A");
    strcpy(dataAgenB.nama, "B");
    strcpy(dataAgenC.nama, "C");
    
    dataAgenA.sharedData = sharedData;
    dataAgenB.sharedData = sharedData;
    dataAgenC.sharedData = sharedData;
    
    dataAgenA.running = false;
    dataAgenB.running = false;
    dataAgenC.running = false;
    
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
    
    printf("Agen A, B, dan C telah dimulai. Tekan Ctrl+C untuk berhenti.\n");
    
    // Loop utama untuk menunggu thread selesai atau sinyal
    int maxWaitTime = 120; // 120 detik (2 menit) timeout total
    int waitCount = 0;
    
    while (!berhenti && waitCount < maxWaitTime) {
        // Periksa apakah semua thread selesai
        if (!dataAgenA.running && !dataAgenB.running && !dataAgenC.running) {
            printf("Semua agen telah selesai mengirimkan pesanan Express\n");
            break;
        }
        
        // Periksa apakah masih ada pesanan Express yang belum dikirim
        if (waitCount % 10 == 0 && !masihAdaPesananExpress(sharedData)) {
            printf("Tidak ada lagi pesanan Express yang belum dikirim.\n");
            // Jangan break disini, biarkan thread selesai sendiri dengan batas waktu
        }
        
        // Tidur untuk mengurangi CPU usage
        sleep(1);
        waitCount++;
    }
    
    // Jika timeout tercapai
    if (waitCount >= maxWaitTime) {
        printf("Timeout tercapai. Menghentikan semua agen.\n");
        berhenti = 1;
        dataAgenA.running = false;
        dataAgenB.running = false;
        dataAgenC.running = false;
    }
    
    printf("Menunggu thread agen untuk bergabung kembali...\n");
    
    // Tunggu thread selesai
    pthread_join(dataAgenA.thread_id, NULL);
    pthread_join(dataAgenB.thread_id, NULL);
    pthread_join(dataAgenC.thread_id, NULL);
    
    // Detach shared memory
    if (shmdt(sharedData) == -1) {
        perror("shmdt");
        exit(EXIT_FAILURE);
    }
    
    printf("Agen pengiriman Express telah berhenti.\n");
    return 0;
}

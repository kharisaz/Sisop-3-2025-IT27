# Sisop-3-2025-IT27

===================[Kelompok IT27]======================

Khumaidi Kharis Az-zacky - 5027241049

Mohamad Arkan Zahir Asyafiq - 5027241120

Abiyyu Raihan Putra Wikanto - 5027241042

============[Laporan Resmi Penjelasan Soal]=============

#soal_1

#soal_2

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

Digunakan saat pemain memulai pertarungan.
Musuh diberikan HP dan reward emas acak.

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

Membuat string seperti [===== ] HP: 50/100 untuk menunjukkan status musuh.
Digunakan saat menyerang musuh.

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
Trik aneh di hasPassive ? (...) : "" dipakai agar bisa menyisipkan deskripsi passive jika senjata punya efek passive (ini sedikit keliru dan bisa dibenahi).
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8080
#define BUF_SIZE    512

void showMenu() {
    puts("\n== WELCOME TO DUNGEON ==");
    puts("1. Show Player Stats");
    puts("2. View Inventory & Equip Weapons");
    puts("3. Shop");
    puts("4. Battle Mode");
    puts("5. Exit");
    printf("Choose an option: ");
}

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

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv = {
        .sin_family = AF_INET,
        .sin_port   = htons(SERVER_PORT)
    };
    inet_pton(AF_INET, SERVER_IP, &serv.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect");
        return 1;
    }
    printf("[CLIENT] Connected to %s:%d\n", SERVER_IP, SERVER_PORT);

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

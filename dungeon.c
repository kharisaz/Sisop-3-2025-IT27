#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <arpa/inet.h>
#include "shop.c"

#define PORT         8080
#define MAX_CLIENTS  10
#define BUF_SIZE     512
#define MAX_INV      20
#define MAX_NAME     32


int      gold[MAX_CLIENTS];
int      invCount[MAX_CLIENTS];
Weapon   inventory[MAX_CLIENTS][MAX_INV];
int      equippedId[MAX_CLIENTS];    
int      kills[MAX_CLIENTS];

int      inBattle[MAX_CLIENTS];
int      enemyHp[MAX_CLIENTS];
int      enemyMaxHp[MAX_CLIENTS];
int      enemyReward[MAX_CLIENTS];

const Weapon FISTS = { -1, "Fists", 0, 5, 0, "" };

static int randRange(int a, int b) {
    return a + rand() % (b - a + 1);
}

void spawnEnemy(int cid) {
    enemyMaxHp[cid]  = randRange(50, 200);
    enemyHp[cid]     = enemyMaxHp[cid];
    enemyReward[cid] = randRange(10, 50);
    inBattle[cid]    = 1;
}

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

void handle_client(int clientSock, int cid) {
    char buf[BUF_SIZE], resp[BUF_SIZE];
    Weapon *shopList; int shopCount;
    getShopList(&shopList, &shopCount);

    gold[cid]        = 100;
    invCount[cid]    = 0;
    equippedId[cid]  = -1;
    kills[cid]       = 0;
    inBattle[cid]    = 0;

    while (1) {
        memset(buf, 0, BUF_SIZE);
        int len = read(clientSock, buf, BUF_SIZE-1);
        if (len<=0) break;
        buf[strcspn(buf, "\r\n")] = 0;

        char cmd[16]; int arg;
        arg = -1;
        sscanf(buf, "%15s %d", cmd, &arg);

        if (strcmp(cmd,"SHOW_STATS")==0) {
            Weapon eq = (equippedId[cid] < 0
                         ? FISTS
                         : inventory[cid][equippedId[cid]]);
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
        else if (strcmp(cmd,"BUY")==0) {
            int found=-1;
            for (int i=0;i<shopCount;i++)
                if (shopList[i].id==arg) { found=i; break; }
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
        else if (strcmp(cmd,"BATTLE")==0) {
            spawnEnemy(cid);
            strcpy(resp,"Enemy appeared!\n");
            buildHpBar(cid, resp+strlen(resp));
            write(clientSock, resp, strlen(resp));
        }
        else if (strcmp(cmd,"ATTACK")==0) {
            if (!inBattle[cid]) {
                strcpy(resp,"You are not in battle. Use BATTLE first.\n");
                write(clientSock, resp, strlen(resp));
            } else {
                Weapon eq = (equippedId[cid]<0?FISTS:inventory[cid][equippedId[cid]]);
                int base = eq.damage;
                int bonus = randRange(0, base);
                int dmg   = base + bonus;
                int isCrit = (randRange(1,100)<=10);
                if (isCrit) dmg*=2;
                int passiveOn = (eq.hasPassive && randRange(1,100)<=20);

                enemyHp[cid] -= dmg;
                int justKilled = (enemyHp[cid] <= 0);
                if (justKilled) {
                    gold[cid] += enemyReward[cid];
                    kills[cid]++;
                }

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
        else if (strcmp(cmd,"EXIT_BATTLE")==0) {
            inBattle[cid]=0;
            strcpy(resp,"Exited battle mode.\n");
            write(clientSock, resp, strlen(resp));
        }
        else if (strcmp(cmd,"EXIT")==0) {
            break;
        }
        else {
            strcpy(resp,"Unknown command.\n");
            write(clientSock, resp, strlen(resp));
        }
    }

    close(clientSock);
    printf("[Client %d] Disconnected.\n", cid);
}

// Struct untuk argumen thread
typedef struct {
    int sock;
    int cid;
} ClientArgs;

void *client_thread(void *arg) {
    ClientArgs *args = (ClientArgs *)arg;
    handle_client(args->sock, args->cid);
    free(args);
    return NULL;
}

//Server
int main() {
    srand((unsigned)time(NULL));

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port   = htons(PORT),
        .sin_addr.s_addr = INADDR_ANY
    };
    bind(serverFd, (struct sockaddr*)&addr, sizeof(addr));
    listen(serverFd, MAX_CLIENTS);
    printf("[SERVER] Listening on port %d...\n", PORT);

    for (int cid=0; cid<MAX_CLIENTS; cid++) {
        invCount[cid]=0;
        inBattle[cid]=0;
    }

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

    close(serverFd);
    return 0;
}

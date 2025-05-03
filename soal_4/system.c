#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>

#define MAX_HUNTERS 50
#define MAX_DUNGEONS 50

struct Hunter {
    char username[50];
    int level;
    int exp;
    int atk;
    int hp;
    int def;
    int banned;
    key_t shm_key;
};

struct Dungeon {
    char name[50];
    int min_level;
    int exp;
    int atk;
    int hp;
    int def;
    key_t shm_key;
};

key_t get_system_key() {
    return ftok("/tmp", 'S');
}

struct Database {
    struct Hunter hunters[MAX_HUNTERS];
    int total_hunters;
    struct Dungeon dungeons[MAX_DUNGEONS];
    int total_dungeons;
};

void hunters_info(struct Database *data) {
    printf("\n====== HUNTER INFO ======\n");
    for (int i=0; i<data->total_hunters; i++) {
        struct Hunter info = data->hunters[i];
        printf("Name: %s\tLevel: %d\tEXP: %d\tATK: %d\tHP: %d\tDEF: %d\tStatus: %s\n", info.username, info.level, info.exp, info.atk, info.hp, info.def, info.banned ? "BANNED" : "ACTIVE");
    }
}

void dungeons_info(struct Database *data) {
    printf("\n====== DUNGEON INFO ======\n");
    for (int i=0; i<data->total_dungeons; i++) {
        struct Dungeon info = data->dungeons[i];
        printf("[Dungeon %d\n]", i+1);
        printf("Name: %s\nMin Lv: %d\nEXP: %d\nATK: %d\nHP: %d\nDEF: %d\nKey: %d\n\n", info.name, info.min_level, info.exp, info.atk, info.hp, info.def, info.shm_key);
    }
}

int random_value(int min, int max) {
    return min + rand() % (max - min + 1);
}

void generate_dungeon(struct Database *data) {
    if (data->total_dungeons >= MAX_DUNGEONS) {
        printf("Dungeon reached the limit.\n");
        return;
    }

    char *names[] = {"Double Dungeon", "Demon Castle", "Pyramid Dungeon", "Red Gate Dungeon", "Hunters Guild Dungeon", "Busan A-Rank Dungeon", "Insects Dungeon", "Goblins Dungeon", "D-Rank Dungeon", "Gwanak Mountain Dungeon", "Hapjeong Subway Station Dungeon"};
    struct Dungeon dgn;
    strcpy(dgn.name, names[rand() % 11]);
    dgn.min_level = random_value(1, 5);
    dgn.atk = random_value(100, 150);
    dgn.hp = random_value(50, 100);
    dgn.def = random_value(25, 50);
    dgn.exp = random_value(150, 300);
    dgn.shm_key = random_value(1, 1000);

    data->dungeons[data->total_dungeons++] = dgn;
    printf("Dungeon generated .\n");
    printf("Nama: %s.\n", dgn.name);
    printf("Minimum Level: %d.\n", dgn.min_level);
}

void ban_hunter(struct Database *data) {
    char name[100];
    printf("Enter target name: ");
    scanf("%s", name);

    for (int i=0; i<data->total_hunters; i++) {
        if (strcmp(data->hunters[i].username, name) == 0) {
            data->hunters[i].banned = !data->hunters[i].banned;
            printf("Hunter %s is now %s.\n", name, data->hunters[i].banned ? "BANNED" : "ACTIVE");
            return;
        }
    }
    printf("Hunter not found.\n");
}

void reset_hunter(struct Database *data) {
    char name[100];
    printf("Enter hunter's name target: ");
    scanf("%s", name);
    for (int i=0; i<data->total_hunters; i++) {
        if (strcmp(data->hunters[i].username, name) == 0) {
            data->hunters[i].level = 1;
            data->hunters[i].exp = 0;
            data->hunters[i].atk = 10;
            data->hunters[i].hp = 100;
            data->hunters[i].def = 5;
            printf("Hunter %s has been reset.\n", name);
            return;
        }
    }
    printf("Hunter not found.\n");
}

int main() {
    srand(time(NULL));
    key_t key = get_system_key();
    int shmid = shmget(key, sizeof(struct Database), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("failed to create or open the share memory");
        exit(1);
    }

    struct Database *data = (struct Database *)shmat(shmid, NULL, 0);
    if (data == (void *) -1) {
        perror("shmat failed");
        exit(1);
    }

    if (data->total_hunters == 0 && data->total_dungeons == 0)
        memset(data, 0, sizeof(struct Database));

    int choice;
    while (1) {
        printf("\n====== SYSTEM MENU ======\n");
        printf("1. Hunter Info\n");
        printf("2. Dungeon Info\n");
        printf("3. Generate Dungeon\n");
        printf("4. Ban Hunter\n");
        printf("5. Reset Hunter\n");
        printf("6. Exit\n");
        printf("Choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: hunters_info(data); break;
            case 2: dungeons_info(data); break;
            case 3: generate_dungeon(data); break;
            case 4: ban_hunter(data); break;
            case 5: reset_hunter(data); break;
            case 6: shmctl(shmid, IPC_RMID, NULL);
                    printf("Exit...\n");
                    return 0;
            default: printf("Invalid choice.\n");
        }
    }
    return 0;
}

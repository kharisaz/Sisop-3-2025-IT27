#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>

#define MAX_HUNTERS 50
#define MAX_DUNGEONS 50

struct Hunter {
    char username[50];
    int level, exp, atk, hp, def, banned;
    key_t shm_key;
};

struct Dungeon {
    char name[50];
    int min_level, exp, atk, hp, def;
    key_t shm_key;
};

struct SystemData {
    struct Hunter hunters[MAX_HUNTERS];
    int num_hunters;
    struct Dungeon dungeons[MAX_DUNGEONS];
    int num_dungeons;
};

key_t get_system_key() {
    return ftok("/tmp", 'S');
}

int find_hunter(struct SystemData *data, const char *username) {
    for (int i=0; i<data->num_hunters; i++)
        if (strcmp(data->hunters[i].username, username) == 0) return i;
    return -1;
}

void register_hunter(struct SystemData *data) {
    if (data->num_hunters >= MAX_HUNTERS) {
        printf("Hunter limit reached.\n");
        return;
    }
    char username[50];
    printf("Username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;
    if (find_hunter(data, username) != -1) {
        printf("Username already exists.\n");
        return;
    }
    struct Hunter h;
    strcpy(h.username, username);
    h.level = 1; h.exp = 0; h.atk = 10; h.hp = 100; h.def = 5; h.banned = 0; h.shm_key = 0;
    data->hunters[data->num_hunters++] = h;
    printf("Registration success!\n");
}

void list_dungeons(struct SystemData *data, int hunter_idx) {
    struct Hunter *hntr = &data->hunters[hunter_idx];
    printf("Available Dungeons:\n");
    int count = 0;
    for (int i=0; i<data->num_dungeons; i++) {
        if (hntr->level >= data->dungeons[i].min_level) {
            struct Dungeon *dgn = &data->dungeons[i];
            printf("%s (Lv %d) -> ATK: %d HP: %d DEF: %d EXP: %d\n", dgn->name, dgn->min_level, dgn->atk, dgn->hp, dgn->def, dgn->exp);
            count++;
        }
    }
    if (count == 0) printf("No dungeons available for your level.\n");
}

void raid_dungeon(struct SystemData *data, int hunter_idx) {
    struct Hunter *hntr = &data->hunters[hunter_idx];
    if (hntr->banned) {
        printf("You are banned and cannot raid.\n");
        return;
    }
    printf("Choose a dungeon to raid:\n");
    int count = 0;
    for (int i=0; i<data->num_dungeons; i++) {
        if (hntr->level >= data->dungeons[i].min_level) printf("%d. %s\n", i+1, data->dungeons[i].name), count++;
    }
    if (!count) {
        printf("No available dungeon for your level.\n");
        return;
    }
    printf("Dungeon number: ");
    int choice; scanf("%d", &choice);
    choice--;
    if (choice<0 || choice >= data->num_dungeons || hntr->level < data->dungeons[choice].min_level) {
        printf("Invalid dungeon choice.\n");
        return;
    }
    struct Dungeon *dgn = &data->dungeons[choice];
    printf("Raiding dungeon %s...\n", dgn->name);
    hntr->exp += dgn->exp; hntr->atk += dgn->atk; hntr->hp += dgn->hp; hntr->def += dgn->def;
    while (hntr->exp >= 500) {
        hntr->level++; hntr->exp -= 500;
        printf("Leveled up! New level: %d\n", hntr->level);
    }
    for (int i=choice; i<data->num_dungeons-1; i++) data->dungeons[i] = data->dungeons[i+1];
    data->num_dungeons--;
    printf("Raid success! Stats increased.\n");
}

void battle_hunter(struct SystemData *data, int hunter_idx) {
    struct Hunter *player = &data->hunters[hunter_idx];
    if (player->banned) {
        printf("You are banned and cannot battle.\n");
        return;
    }
    printf("Choose opponent:\n");
    int count = 0;
    for (int i=0; i<data->num_hunters; i++) {
        if (i != hunter_idx && !data->hunters[i].banned)
            printf("%d. %s (Lv %d)\n", i+1, data->hunters[i].username, data->hunters[i].level), count++;
    }
    if (!count) {
        printf("No valid opponents.\n");
        return;
    }

    printf("Opponent number: ");
    int choice; scanf("%d", &choice);
    choice--;
    if (choice<0 || choice>=data->num_hunters || choice==hunter_idx) {
        printf("Invalid opponent choice.\n");
        return;
    }

    struct Hunter *opponent = &data->hunters[choice];
    int player_power = player->atk + player->hp + player->def;
    int opponent_power = opponent->atk + opponent->hp + opponent->def;
    printf("You chose to battle %s\n", opponent->username);
    printf("Your power: %d\n", player_power);
    printf("Opponent's power: %d\n", opponent_power);

    if(player_power >= opponent_power) {
        player->atk += opponent->atk; player->hp += opponent->hp; player->def += opponent->def; player->exp += opponent->exp;
        while(player->exp >= 500) { player->level++; player->exp -= 500; printf("Leveled up! Level %d\n", player->level); }
        printf("Deleting %s's shared memory\n", opponent->username);
        for(int i=choice; i<data->num_hunters-1; i++) data->hunters[i] = data->hunters[i+1];
        data->num_hunters--;
        printf("Battle won, you acquired %s's stats\n", opponent->username);
    } else {
        opponent->atk += player->atk; opponent->hp += player->hp; opponent->def += player->def; opponent->exp += player->exp;
        while(opponent->exp >= 500) { opponent->level++; opponent->exp -= 500; printf("%s leveled up to %d\n", opponent->username, opponent->level);}
        printf("You lose, Deleting %s's shared memory\n", player->username);
        for(int i=hunter_idx; i<data->num_hunters-1; i++) data->hunters[i]=data->hunters[i+1];
        data->num_hunters--;
        printf("you are acquired\n");
    }
}

void toggle_notification(struct SystemData *data, int hunter_idx) {
    printf("Dungeon notification: Press 'e' + ENTER to stop.\n");
    while(1) {
        system("clear");
        printf("Dungeon Notification for %s\n", data->hunters[hunter_idx].username);
        if (data->num_dungeons == 0) printf("No dungeons available.\n");
        else for (int i=0; i<data->num_dungeons; i++) {
            struct Dungeon *dgn = &data->dungeons[i];
            printf("%s (Lv %d) ATK:%d HP:%d DEF:%d EXP:%d\n", dgn->name, dgn->min_level, dgn->atk, dgn->hp, dgn->def, dgn->exp);
        }

        printf("\nPress 'e' + ENTER to exit, or wait 3 seconds for next update.\n");
        fd_set set; struct timeval timeout;
        FD_ZERO(&set); FD_SET(STDIN_FILENO, &set);
        timeout.tv_sec = 3; timeout.tv_usec = 0;
        int sel = select(STDIN_FILENO+1, &set, NULL, NULL, &timeout);
        if(sel > 0) {
            char c = getchar();
            if(c == 'e' || c == 'E') break;
        }
    }
}

int main() {
    srand(time(NULL));
    key_t key = get_system_key();
    int shmid = shmget(key, sizeof(struct SystemData), 0666);
    if (shmid < 0) {
        perror("shmget failed");
        exit(1);
    }

    struct SystemData *data = shmat(shmid, NULL, 0);
    if (data == (void *)-1) {
        perror("shmat failed");
        exit(1);
    }

    while(1) {
        printf("\n====== HUNTER MENU ======\n1. Register\n2. Login\n3. Exit\nChoice: ");
        int choice; scanf("%d", &choice); getchar();
        if(choice == 1) register_hunter(data);
        else if(choice == 2) {
            char username[50];
            printf("Username: ");
            fgets(username, sizeof(username), stdin);
            username[strcspn(username, "\n")] = 0;
            int idx = find_hunter(data, username);
            if(idx == -1) {
                printf("Login failed.\n");
                continue;
            }
            if(data->hunters[idx].banned) {
                printf("You are banned.\n");
                continue;
            }
            while(1) {
                printf("\n--- %s's MENU ---\n1. List Dungeon\n2. Raid\n3. Battle\n4. Toggle Notification\n5. Exit\nChoice: ", data->hunters[idx].username);
                int sc; scanf("%d", &sc); getchar();
                if(sc == 1) list_dungeons(data, idx);
                else if(sc == 2) raid_dungeon(data, idx);
                else if(sc == 3) battle_hunter(data, idx);
                else if(sc == 4) toggle_notification(data, idx);
                else if(sc == 5) break;
                else printf("Invalid choice.\n");
            }
        }
        else if(choice == 3) break;
        else printf("Invalid choice.\n");
    }
    return 0;
}

#include <string.h>
#include "shop.h"

static Weapon weapons[MAX_WEAPONS] = {
    { 0, "Iron Sword",    20,  8, 0, "" },
    { 1, "Steel Axe",     30, 10, 1, "+10% Crit Chance" },
    { 2, "Magic Wand",    50, 12, 1, "Chance to Silence" },
    { 3, "Wooden Club",   10,  5, 0, "" },
    { 4, "Dragon Spear",  80, 15, 0, "" },
    { 5, "Shadow Dagger", 35,  9, 1, "+20% Attack Speed" }
};

void getShopList(Weapon **list, int *count) {
    *list  = weapons;
    *count = MAX_WEAPONS;
}

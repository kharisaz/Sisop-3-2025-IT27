#ifndef SHOP_H
#define SHOP_H

#define MAX_WEAPONS 6
#define NAME_LEN    32
#define PASS_LEN    64

typedef struct {
    int  id;
    char name[NAME_LEN];
    int  price;
    int  damage;
    int  hasPassive;           // 0 = no, 1 = yes
    char passiveDesc[PASS_LEN];
} Weapon;

/**
 * Mengembalikan pointer ke array senjata (length = *count).
 * Array bersifat static—jangan di-free.
 */
void getShopList(Weapon **list, int *count);

#endif // SHOP_H

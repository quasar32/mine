#ifndef HOTBAR
#define HOTBAR

struct hotbar_slot {
    int id;
    int n; 
};

extern struct hotbar_slot hotbar_slots[10];
extern int hotbar_sel;

/**
 * add_to_hotbar() - Add item to hotbar
 * @id: ID of item 
 *
 * Return: Zero if hotbar is full
 */
int add_to_hotbar(int id);

#endif 

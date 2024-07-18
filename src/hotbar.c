#include "hotbar.h"

struct hotbar_slot hotbar_slots[10];
int hotbar_sel;

int add_to_hotbar(int id) {
    int i;
    struct hotbar_slot *slot, *empty;

    empty = 0;
    for (i = 0; i < 10; i++) {
        slot = hotbar_slots + i;
        if (slot->id == id && 
            slot->n > 0 && slot->n < 64) {
            slot->n++;
            return 1;
        }
        if (!empty && !slot->n) {
            empty = slot;
        }
    }
    if (!empty) {
        return 0;
    }    
    empty->id = id;
    empty->n = 1;
    return 1;
}

#include "os.h"

void page_table_update(uint64_t pt, uint64_t vpn, uint64_t ppn){
    uint64_t curr_physical_add = pt << 12;
    uint64_t *pointer;
    int destroy = ppn == NO_MAPPING;
    uint64_t next_padd;
    uint64_t tmp;
    for(int i=4; i>=0; i--){
        pointer = phys_to_virt(curr_physical_add);
        tmp = (vpn >> (9*i)) & 0x1ff; //0b111111111
        next_padd = pointer[tmp];
        uint64_t valid = next_padd & (uint64_t)1;
        if (valid == 0){
            if (destroy){
                return;
            }else{
                pointer[tmp] = (alloc_page_frame() << 12)+1;
            }
        }
        curr_physical_add = pointer[tmp] - 1;
    }

    if (destroy){
        pointer[tmp] = pointer[tmp]-1;
    }else{
        pointer[tmp] = (ppn << 12)+1;
    }
}



uint64_t page_table_query(uint64_t pt, uint64_t vpn){
    
    uint64_t curr_physical_add = pt << 12;
    uint64_t *pointer;
    
    for(int i=4; i>=0; i--){
        pointer = phys_to_virt(curr_physical_add);
        uint64_t tmp = (vpn >> (9*i)) & 0x1ff; //0b111111111
        curr_physical_add = pointer[tmp];
        uint64_t valid = curr_physical_add & (uint64_t)1;
        if (valid == 0){
            return NO_MAPPING;
        }
        curr_physical_add = curr_physical_add-1;

    }

    return curr_physical_add >> 12;

    

}
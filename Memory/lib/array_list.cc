#include "array_list.h"
#include <assert.h>
#include "memory_manager.h"

namespace proj3 {
    ArrayList::ArrayList(size_t sz, MemoryManager* cur_mma, int id){
        this->size = sz;
        this->mma = cur_mma;
        this->array_id = id;
    }
    int ArrayList::Read (unsigned long idx){
        //read the value in the virtual index of 'idx' from mma's memory space
        int virtual_page_id = idx / PageSize;
        int offset = idx - virtual_page_id * PageSize;
        return mma->ReadPage(array_id, virtual_page_id, offset);
    }
    void ArrayList::Write (unsigned long idx, int value){
        //write 'value' in the virtual index of 'idx' into mma's memory space
        int virtual_page_id = idx / PageSize;
        int offset = idx - virtual_page_id * PageSize;
        // printf("virtual_page_id: %d offset: %d\n", virtual_page_id, offset);
        // getchar();
        mma->WritePage(array_id, virtual_page_id, offset, value);
    }
    ArrayList::~ArrayList(){
    }
} // namespce: proj3
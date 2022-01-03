#include "memory_manager.h"
#include <cstdio>
#include <cstdlib>
#include <assert.h>
#include "array_list.h"
#include <mutex>
#include <map>
#include <cmath>
#include <condition_variable>

namespace proj4 {
    std::string CalcFileString(int block_number) {
        char *intstr = new char[1 + (int)(log(block_number)/log(10))];
        sprintf(intstr, "%d", block_number);
        std::string blocknum = intstr;
        // Please change the path of disk
        std::string FileString = "/home/owner/Documents/2021Fall-OS/my_code/Memory/disk/" + blocknum + ".txt";
        return FileString;
    }
    void ClearBlock(int blockn) {
        FILE *fp;
        std::string filename = CalcFileString(blockn);
        const char * s = filename.c_str();
        
        fp = fopen(s, "r");
        if (fp != NULL) {
            fclose(fp);
            fp = fopen(s, "w");
            for (int i = 0; i < PageSize; ++i)
                fprintf(fp, "0 ");
            fclose(fp);
        }
    }

    PageFrame::PageFrame(){
    }
    int& PageFrame::operator[] (unsigned long idx){
        //each page should provide random access like an array
        return mem[idx];
    }
    void PageFrame::WriteDisk(std::string filename) {
        // write page content into disk files
        FILE *fp;
        const char * s = filename.c_str();
        fp = fopen(s, "w");
        assert(fp);
        
        for (int i = 0; i < PageSize; ++i)
            fprintf(fp, "%d ", mem[i]);

        fclose(fp);
    }
    void PageFrame::ReadDisk(std::string filename) {
        // read page content from disk files
        FILE *fp;
        const char * s = filename.c_str();
        fp = fopen(s, "r");
        if (fp == NULL) {
            for (int i = 0; i < PageSize; ++i)
                mem[i] = 0;
            return;
        }

        for (int i = 0; i < PageSize; ++i)
            fscanf(fp, "%d", &mem[i]);

        fclose(fp);
    }

    PageInfo::PageInfo(){
    }
    void PageInfo::SetInfo(int cur_holder, int cur_vid, int block_number){
        //modify the page states
        //you can add extra parameters if needed
        this->holder = cur_holder;
        this->virtual_page_id = cur_vid;
        this->block_number = block_number;
    }
    void PageInfo::ClearInfo(){
        //clear the page states
        //you can add extra parameters if needed
        this->dirty = 0;
        this->holder = 0;
        this->virtual_page_id = 0;
        this->block_number = 0;
        this->counter = 0;
        this->usebit = 0;
    }
    void PageInfo::Dirty(){
        //modify the page states
        //you can add extra parameters if needed
        this->dirty = 1;
    }
    bool PageInfo::IsDirty(){
        //modify the page states
        //you can add extra parameters if needed
        return this->dirty;
    }
    void PageInfo::Access(){
        this->usebit = 1;
    }
    std::string PageInfo::GetFileString(){
        return CalcFileString(this->block_number);
    }

    int PageInfo::GetHolder(){
        return this->holder;
    }
    int PageInfo::GetVid(){
        return this->virtual_page_id;
    }
    int PageInfo::GetBid(){
        return this->block_number;
    }
    bool PageInfo::Check() {
        if (this->usebit == 0) {
            this->counter ++;
            if(this->counter == NN)
                return true;
        }
        else {
            this->usebit = 0;
            this->counter = 0;
        }
        return false;
    }

    MemoryManager::MemoryManager(size_t sz, int max_vir_page_num){
        //mma should build its memory space with given space size
        //you should not allocate larger space than 'sz' (the number of physical pages) 
        mem = new PageFrame[sz];
        mma_sz = sz;
        max_vir_page_num_limit = max_vir_page_num;
        next_array_id = 0;
        next_block_number = 1;
        pt = 0;
        in_use_page_number.lock();
        in_use_page_num =0;
        in_use_page_number.unlock();
        free_list = new unsigned int[sz];
        page_info = new PageInfo[sz];
        for (unsigned int i = 0; i < sz; ++i) {
            free_list[i] = 1;
            page_info[i].ClearInfo();
        }
    }
    MemoryManager::~MemoryManager(){
        delete mem;
        delete free_list;
        delete page_info;
    }
    void MemoryManager::PageOut(int physical_page_id){
        //swap out the physical page with the indx of 'physical_page_id out' into a disk file
        assert(free_list[physical_page_id] == 0);
        // printf("%d %d\n",physical_page_id, page_info[physical_page_id].GetBid());
        // getchar();
        if (page_info[physical_page_id].IsDirty())
            mem[physical_page_id].WriteDisk(page_info[physical_page_id].GetFileString());

        page_map[page_info[physical_page_id].GetHolder()][page_info[physical_page_id].GetVid()] = -page_info[physical_page_id].GetBid();

        page_info[physical_page_id].ClearInfo();
        free_list[physical_page_id] = 1;
    }
    void MemoryManager::PageIn(int array_id, int virtual_page_id, int physical_page_id){
        //swap the target page from the disk file into a physical page with the index of 'physical_page_id out'
        assert(free_list[physical_page_id]);
        free_list[physical_page_id] = 0;
        // printf("%d\n", -page_map[array_id][virtual_page_id]);
        // getchar();
        mem[physical_page_id].ReadDisk(CalcFileString(-page_map[array_id][virtual_page_id]));

        page_info[physical_page_id].SetInfo(array_id, virtual_page_id, -page_map[array_id][virtual_page_id]);
        page_map[array_id][virtual_page_id] = physical_page_id;
    }
    void MemoryManager::PageReplace(int array_id, int virtual_page_id){
        //implement your page replacement policy here

        //FIFO
        // if (free_list[pt]) {
        //     PageIn(array_id, virtual_page_id, pt);
        // }
        // else {
        //     // printf("%d\n",pt);
        //     // getchar();
        //     PageOut(pt);
        //     PageIn(array_id, virtual_page_id, pt);
        // }
        // // if(virtual_page_id == 1) printf("Arrive!\n");
        // pt = (pt + 1) % mma_sz;

        // Clock
        for (int i = 0; i < mma_sz; ++i)
            if (free_list[i]) {
                PageIn(array_id, virtual_page_id, i);
                return;
            }
        for (;;++pt) {
            if(pt >= mma_sz) pt -= mma_sz;
            if(page_info[pt].Check()) {
                PageOut(pt);
                PageIn(array_id, virtual_page_id, pt);
                pt = (pt + 1) % mma_sz;
                return;
            }
        }
    }
    int MemoryManager::ReadPage(int array_id, int virtual_page_id, int offset){
        // for arrayList of 'array_id', return the target value on its virtual space
        lock.write_lock();
        auto itr = page_map.find(array_id);
        assert(itr != page_map.end());
        auto it = itr->second.find(virtual_page_id);
        assert(it != itr->second.end());

        int physical_page_id = it->second;
        if (physical_page_id < 0) {
            PageReplace(array_id, virtual_page_id);
            physical_page_id = page_map[array_id][virtual_page_id];
        }
        else page_info[physical_page_id].Access();
        int res = mem[physical_page_id][offset];
        lock.write_unlock();
        return res;
    }
    void MemoryManager::WritePage(int array_id, int virtual_page_id, int offset, int value){
        // for arrayList of 'array_id', write 'value' into the target position on its virtual space
        lock.write_lock();
        //printf("array_id %d vpi %d offset %d\n", array_id, virtual_page_id, offset);
        auto itr = page_map.find(array_id);
        // if (itr == page_map.end()) {
        //     printf("array_id %d vpi %d offset %d\n", array_id, virtual_page_id, offset);
        //     for (auto x = page_map.begin(); x != page_map.end(); x++)
        //         printf("%d ", x->first);
        //     printf("\n");
        // }
        assert(itr != page_map.end());
        auto it = itr->second.find(virtual_page_id);
        // if (it == itr->second.end()) {
        //     printf("array_id %d vpi %d offset %d\n", array_id, virtual_page_id, offset);
        //     for (auto x = itr->second.begin(); x != itr->second.end(); x++)
        //         printf("%d ", x->first);
        //     printf("\n");
        // }
        assert(it != itr->second.end());
        int physical_page_id = it->second;
        if (physical_page_id < 0) {
            PageReplace(array_id, virtual_page_id);
            physical_page_id = page_map[array_id][virtual_page_id];
        }
        else page_info[physical_page_id].Access();
        if (mem[physical_page_id][offset] != value) {
            page_info[physical_page_id].Dirty();
            mem[physical_page_id][offset] = value;
        }
        lock.write_unlock();
        return;
    }
    int MemoryManager::Allocate(size_t sz){ // in byte
        // when an application requires for memory, create an ArrayList and record mappings from its virtual memory space to the physical memory space
        //printf("Start Allocation in memory_manager\n");
        int number_of_pages = sz / PageSize;
        if (sz > number_of_pages * PageSize)
            number_of_pages += 1;
        in_use_page_number.lock();
        if (max_vir_page_num_limit != -1 && number_of_pages + in_use_page_num > max_vir_page_num_limit) {
            in_use_page_number.unlock();
            return -1;
        }
        in_use_page_num += number_of_pages;
        in_use_page_number.unlock();
        lock.write_lock();
        int res = next_array_id ++;
        int tmp = next_block_number;
        next_block_number += number_of_pages;
        int loc = next_array_id - 1;

        for(int i = 0; i < number_of_pages; ++i) {
            page_map[loc][i] = -(tmp+i);
            // Clear block !!!!!
            ClearBlock(tmp+i);
        }
        lock.write_unlock();
        //printf("%d\t", in_use_page_num,"Finish Allocation in memory_manager\n");
        return res;
    }
    void MemoryManager::Release(int array_id){
        // an application will call release() function when destroying its arrayList
        // release the virtual space of the arrayList and erase the corresponding mappings
        in_use_page_number.lock();
        in_use_page_num -= page_map[array_id].size();
        in_use_page_number.unlock();
        lock.write_lock();
        page_map.erase(array_id);
        lock.write_unlock();
    }
} // namespce: proj3
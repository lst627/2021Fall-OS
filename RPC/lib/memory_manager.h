#ifndef MEMORY_MANAGER_H_
#define MEMORY_MANAGER_H_

#include <map>
#include <string>
#include <cstdlib>
#include<cstdio>
#include <assert.h>
#include <mutex>
#include <condition_variable>
#include <cmath>

#define PageSize 1024
#define NN 1

namespace proj4 {

class RWLock {
    public:
        int AR=0, WW=0, AW=0, WR=0;
        std::mutex *mtx;
        std::condition_variable *okread;
        std::condition_variable *okwrite;
        RWLock(){
            mtx = new std::mutex;
            okread = new std::condition_variable;
            okwrite = new std::condition_variable;
        }

        ~RWLock() {
            delete mtx;
            delete okread;
            delete okwrite;
        }
        
        void read_lock() {
            std::unique_lock<std::mutex> lck(*mtx);
            //printf("Start read lock\n");
            while ((this->AW + this->WW) > 0) {
                this->WR ++;
                this->okread->wait(lck);
                this->WR --;
            }
            this->AR ++;
            lck.unlock();
            //printf("Finish read lock\n");
        }

        void read_unlock() { 
            std::unique_lock<std::mutex> lck(*mtx);
            //printf("Start read unlock\n");
            this->AR --;
            if(this->AR == 0 && this->WW > 0)
                this->okwrite->notify_all();
            lck.unlock();
            //printf("Finish read unlock\n");
        }

        void write_lock() {
            std::unique_lock<std::mutex> lck(*mtx);
            //printf("Start write lock\n");
            while ((this->AW + this->AR) > 0) {
                this->WW ++;
                this->okwrite->wait(lck);
                this->WW --;
            }
            this->AW ++;
            lck.unlock();
            //printf("Finish write lock\n");
        }

        void write_unlock() {
            std::unique_lock<std::mutex> lck(*mtx);
            //printf("Start write unlock\n");
            this->AW --;
            if (this->WW > 0)
                this->okwrite->notify_all();
            else if (this->WR > 0)
                this->okread->notify_all();
            lck.unlock();
            //printf("Finish write unlock\n");
        }

    private:
        
        //volatile std::atomic<int> AR=0, WW=0, AW=0, WR=0; is a possible choice
};

class PageFrame {
public:
    PageFrame();
    int& operator[] (unsigned long);
    void WriteDisk(std::string);
    void ReadDisk(std::string);
private:
    int mem[PageSize];
};

class PageInfo {
public:
    PageInfo();
    void SetInfo(int,int,int);
    void ClearInfo();
    void Dirty();
    void Access();
    bool IsDirty();
    bool Check();
    int GetHolder();
    int GetVid();
    int GetBid();
    std::string GetFileString();
private:
    int holder; //page holder id (array_id)
    int virtual_page_id; // page virtual #
    /*add your extra states here freely for implementation*/
    int dirty; // whether the page is dirty
    int block_number; // address in disk
    int usebit;
    int counter;
};

class ArrayList;

class MemoryManager {
public:
    // you should not modify the public interfaces used in tests
    MemoryManager(size_t, int);
    int ReadPage(int array_id, int virtual_page_id, int offset);
    void WritePage(int array_id, int virtual_page_id, int offset, int value);
    int Allocate(size_t);
    void Release(int array_id);
    ~MemoryManager();
    RWLock lock = RWLock(); // lock
private:
    std::map<int, std::map<int, int>> page_map; // // mapping from ArrayList's virtual page # to physical page #
    PageFrame* mem; // physical pages, using 'PageFrame* mem' is also acceptable 
    PageInfo* page_info; // physical page info
    unsigned int* free_list;  // use bitmap implementation to identify and search for free pages
    int next_array_id;
    size_t mma_sz;
    int next_block_number;
    int pt; // pointer that points to the earliest page in memory
    /*add your extra states here freely for implementation*/
    int max_vir_page_num_limit; // record max virtual memory page number
    std::mutex in_use_page_number; // a mutex to guarantee 'in_use_page_num' to be updated thread-safely
    int in_use_page_num; // record the existing usage virtual memory page number
    void PageIn(int array_id, int virtual_page_id, int physical_page_id);
    void PageOut(int physical_page_id);
    void PageReplace(int array_id, int virtual_page_id);
};

}  // namespce: proj3

#endif


#ifndef THREAD_LIB_EMBEDDING_H_
#define THREAD_LIB_EMBEDDING_H_

#include <string>
#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

namespace proj1 {

enum EMBEDDING_ERROR {
    LEN_MISMATCH = 0,
    NON_POSITIVE_LEN
};

class RWLock {
    public:
        RWLock(){}

        ~RWLock() {}
        
        void read_lock() {
            std::unique_lock<std::mutex> lck(mtx);
            while ((this->AW + this->WW) > 0) {
                this->WR ++;
                this->okread.wait(lck);
                this->WR --;
            }
            this->AR ++;
            lck.unlock();
        }

        void read_unlock() { 
            std::unique_lock<std::mutex> lck(mtx);
            this->AR --;
            if(this->AR == 0 && this->WW > 0)
                this->okwrite.notify_all();
            lck.unlock();
        }

        void write_lock() {
            std::unique_lock<std::mutex> lck(mtx);
            while ((this->AW + this->AR) > 0) {
                this->WW ++;
                this->okwrite.wait(lck);
                this->WW --;
            }
            this->AW ++;
            lck.unlock();
        }

        void write_unlock() {
            std::unique_lock<std::mutex> lck(mtx);
            this->AW --;
            if (this->WW > 0)
                this->okwrite.notify_all();
            else if (this->WR > 0)
                this->okread.notify_all();
            lck.unlock();
        }

    private:
        std::mutex mtx;
        std::condition_variable okread;
        std::condition_variable okwrite;
        int AR=0, WW=0, AW=0, WR=0;
        //volatile std::atomic<int> AR=0, WW=0, AW=0, WR=0; is a possible choice
};

using RWLocker = RWLock*;

class Embedding{
public:
    Embedding(){}
    Embedding(int);  // Random init an embedding
    Embedding(int, double*);
    Embedding(int, std::string);
    Embedding(Embedding*);
    ~Embedding() { delete []this->data; }
    double* get_data() { return this->data; }
    int get_length() { return this->length; }
    void update(Embedding*, double);
    std::string to_string();
    void write_to_stdout();
    // Operators
    Embedding operator+(const Embedding&);
    Embedding operator+(const double);
    Embedding operator-(const Embedding&);
    Embedding operator-(const double);
    Embedding operator*(const Embedding&);
    Embedding operator*(const double);
    Embedding operator/(const Embedding&);
    Embedding operator/(const double);
    bool operator==(const Embedding&);
private:
    int length;
    double* data;
    RWLocker lock;
};

using EmbeddingMatrix = std::vector<Embedding*>;
using EmbeddingGradient = Embedding;

class EmbeddingHolder{
public:
    EmbeddingHolder(std::string filename);
    EmbeddingHolder(EmbeddingMatrix &data);
    ~EmbeddingHolder();
    static EmbeddingMatrix read(std::string);
    void write_to_stdout();
    void write(std::string filename);
    int append(Embedding *data);
    void update_embedding(int, EmbeddingGradient*, double);
    Embedding* get_embedding(int idx) const { return this->emb_matx[idx]; } 
    unsigned int get_n_embeddings() { return this->emb_matx.size(); }
    int get_emb_length() {
        return this->emb_matx.empty()? 0: this->get_embedding(0)->get_length();
    }
    bool operator==(const EmbeddingHolder&);
private:
    EmbeddingMatrix emb_matx;
    RWLocker lock;
};

} // namespace proj1
#endif // THREAD_LIB_EMBEDDING_H_

#include <vector>
#include <tuple>

#include <string>   // string
#include <chrono>   // timer
#include <iostream> // cout, endl
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <typeinfo>

#include "lib/utils.h"
#include "lib/model.h" 
#include "lib/embedding.h" 
#include "lib/instruction.h"

namespace proj1 {

void Coldstart(EmbeddingHolder* users, Embedding* new_user, Embedding* item_emb, int user_idx) {
    // Call cold start for downstream applications, slow

    EmbeddingGradient* gradient = cold_start(new_user, item_emb);

    new_user->lock.write_lock();
    users->update_embedding(user_idx, gradient, 0.01);
    new_user->lock.write_unlock();
    delete gradient;
}

void run_one_instruction(Instruction inst, EmbeddingHolder* users, EmbeddingHolder* items, std::vector<int*>* counter, proj1::RWLock* counter_lock, std::vector<proj1::RWLock*>* epoch_lock, int max_epoch) {
    switch(inst.order) {
        case INIT_EMB: {
            // We need to init the embedding
            users->lock.read_lock();
            int length = users->get_emb_length();
            users->lock.read_unlock();

            Embedding* new_user = new Embedding(length);

            users->lock.write_lock();
            int user_idx = users->append(new_user);
            users->lock.write_unlock();

            std::vector<std::thread *> multiple_coldstarts;
            
            for (int item_index: inst.payloads) {
                Embedding* item_emb = items->get_embedding(item_index);
                std::thread *t = new std::thread(Coldstart, users, new_user, item_emb, user_idx);
                multiple_coldstarts.push_back(t);
            }
            int len = multiple_coldstarts.size();
            for (int j=0;j<len;++j) 
                multiple_coldstarts[j]->join();

            break;
        }
        case UPDATE_EMB: {
            int user_idx = inst.payloads[0];
            int item_idx = inst.payloads[1];
            int label = inst.payloads[2];
            // You might need to add this state in other questions.
            // Here we just show you this as an example
            int epoch = -1;
            if (inst.payloads.size() > 3) {
               epoch = inst.payloads[3];
            }
            if (epoch != -1) {
                epoch_lock->at(epoch)->read_lock();
                printf("Started! epoch = %d\n", epoch);
            }

            Embedding* user = users->get_embedding(user_idx);
            Embedding* item = items->get_embedding(item_idx);
            
            std::future<proj1::EmbeddingGradient*> t_user = std::async(proj1::calc_gradient, user, item, label);
            std::future<proj1::EmbeddingGradient*> t_item = std::async(proj1::calc_gradient, item, user, label);
            
            EmbeddingGradient* gradient_user = t_user.get();
            EmbeddingGradient* gradient_item = t_item.get();

            user->lock.write_lock();
            item->lock.write_lock();
            items->update_embedding(item_idx, gradient_user, 0.001);
            users->update_embedding(user_idx, gradient_item, 0.01);
            item->lock.write_unlock();
            user->lock.write_unlock();
            delete gradient_user;
            delete gradient_item;

            if (epoch != -1) {
                printf("Finished! epoch = %d\n", epoch);
                epoch_lock->at(epoch)->read_unlock();
                counter_lock->write_lock();
                (*(counter->at(epoch))) --;
                if ((*(counter->at(epoch))) == 0) {
                    for(int i=epoch+1;i<=max_epoch;++i) {
                        epoch_lock->at(i)->write_unlock();
                        if ((*(counter->at(i))) != 0) break;
                    }
                }
                counter_lock->write_unlock();
            }
            break;
        }
        case RECOMMEND: {
            int user_idx = inst.payloads[0];
            std::vector<Embedding*> item_pool;
            int iter_idx = inst.payloads[1] + 1;
            while(true) {
                counter_lock->read_lock();
                bool flag = true;
                if(iter_idx > 0) {
                    for(int i=0;i<iter_idx;++i) {
                        if((*(counter->at(i))) != 0) {
                            flag = false;
                            break;
                        }
                    }
                }
                if(iter_idx == 0 || flag == true) {
                    counter_lock->read_unlock();
                    printf("Recommend! epoch = %d\n", iter_idx);
                    proj1::EmbeddingHolder* users_copy = users;
                    proj1::EmbeddingHolder* items_copy = items;
                    Embedding* user = users_copy->get_embedding(user_idx);
                    for (unsigned int i = 2; i < inst.payloads.size(); ++i) {
                    int item_idx = inst.payloads[i];
                    item_pool.push_back(items_copy->get_embedding(item_idx));
                    }
                Embedding* recommendation = recommend(user, item_pool);
                recommendation->write_to_stdout();
                break;
                }
                else {
                    counter_lock->read_unlock();
                }
            }
            
            //epoch_lock->at(iter_idx)->read_unlock();
            break;
        }
    }

}

} // namespace proj1

int main(int argc, char *argv[]) {

    proj1::EmbeddingHolder* users = new proj1::EmbeddingHolder("data/q4.in");
    proj1::EmbeddingHolder* items = new proj1::EmbeddingHolder("data/q4.in");
    proj1::Instructions instructions = proj1::read_instructrions("data/q4_instruction.tsv");
    {
    proj1::AutoTimer timer("q5");  // using this to print out timing of the block

    // Preprocesssing
    int max_epoch = -1;
    for (proj1::Instruction inst: instructions) {
        if (inst.order == proj1::UPDATE_EMB) continue;
        int epoch = -1;
        if (inst.payloads.size() > 3) {
            epoch = inst.payloads[3];
        }
        max_epoch = epoch > max_epoch ? epoch : max_epoch;
    }
    std::vector<int*> counter;
    std::vector<proj1::RWLock*> epoch_lock;
    proj1::RWLock *counter_lock = new proj1::RWLock;
    for (int i=0;i<=max_epoch;++i) {
        epoch_lock.push_back(new proj1::RWLock);
        epoch_lock[i]->write_lock();
        counter.push_back(new int(0));
    }
    for (proj1::Instruction inst: instructions) {
        if (inst.order == proj1::INIT_EMB) continue;
        if (inst.order == proj1::UPDATE_EMB) {
            int epoch = -1;
            if (inst.payloads.size() > 3) {
                epoch = inst.payloads[3];
            }
            if(epoch >= 0) 
                (*counter[epoch]) ++;
        }
        else {
            int epoch = inst.payloads[1] + 1;
            max_epoch = epoch > max_epoch ? epoch : max_epoch;
        }
    }
    for (int i=0;i<=max_epoch;++i) {
        epoch_lock[i]->write_unlock();
        if ((*counter[i]) != 0) break;
    }
    // printf("%d\n", max_epoch);

    // Run all the instructions
    std::vector<std::thread *> threadArr;
    for (proj1::Instruction inst: instructions) {
        //printf("%d\n", threadArr.size());
        std::thread *t = new std::thread(proj1::run_one_instruction, inst, users, items, &counter, counter_lock, &epoch_lock, max_epoch);
        threadArr.push_back(t);
    }
    int len = threadArr.size();
    for (int j=0;j<len;++j) 
        threadArr[j]->join();
    delete counter_lock;
    for (int i=0;i<=max_epoch;++i) {
        delete counter[i];
        delete epoch_lock[i];
    }
    }

    // Write the result
    users->write_to_stdout();
    items->write_to_stdout();

    // We only need to delete the embedding holders, as the pointers are all
    // pointing at the emb_matx of the holders.
    delete users;
    delete items;

    return 0;
}
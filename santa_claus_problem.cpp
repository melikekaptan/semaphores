#include<semaphore>
#include<thread>

#include"util.h"

int reindeerCount = 0;  
int elfCount = 0;  

constexpr int NUM_REINDEER = 9;
constexpr int NUM_ELVES    = 10;   // total elves in the system
constexpr int GROUP_SIZE   = 3;    // exactly 3 elves must arrive to wake Santa
constexpr int ELF_WORK_ROUNDS = 3;
constexpr int REINDEER_VACATION_ROUNDS = 2;
constexpr double SLOWDOWN_FACTOR = 10.0;

std::mutex mtx;
static std::counting_semaphore<9999> onlyElves(3);
static std::counting_semaphore<9999> reindeerSem(4);
static std::counting_semaphore<9999> santaSem(0);
static std::counting_semaphore<9999> doubleReindeerSem(0);
static std::counting_semaphore<9999> santaSignal(2);
static std::counting_semaphore<9999> problem(3);
static std::counting_semaphore<9999> elfDone(3);


void ElfThread(int id){
    for(int round = 0; round < ELF_WORK_ROUNDS; round++){

        utils::SafePrint("[Elf " + std::to_string(id) + "] Making toys");
        utils::RandomSleep(300, 600, SLOWDOWN_FACTOR);


        bool hasProblem = (rand()% 100<30);
        if(!hasProblem)
            continue;

        onlyElves.acquire();

        {
            std::unique_lock lock(mtx);
            elfCount++;
            utils::SafePrint("[Elf " + std::to_string(id) + "] Has a problem! ElfCount="+ std::to_string(elfCount));

            if(elfCount == GROUP_SIZE)
            {
                utils::SafePrint("[Elf " + std::to_string(id) + "] I am the 3rd elf, waking Santa");
                santaSem.release();
            }
            else{
                utils::SafePrint("[Elf " + std::to_string(id) + "] Waking outside for the group of 3rd");
            }
        }
        // If I'm NOT the 3rd, I block on santaSignal
        if (elfCount < GROUP_SIZE) {
            santaSignal.acquire();
        }

        // Wait for Santa to say "go ahead" (problem)
        problem.acquire();

        // Now talk to Santa
        utils::SafePrint("[Elf " + std::to_string(id) + "] Asking Santa my question...");
        utils::RandomSleep(200, 400, SLOWDOWN_FACTOR);

        // Wait until Santa's done helping
        elfDone.acquire();

        utils::SafePrint("[Elf " + std::to_string(id) + "] Done with Santa. Returning to work.");

        // Release slot so next elf can form a group
        onlyElves.release();
    }
    utils::SafePrint("[Elf " + std::to_string(id) + "] Done with all rounds, exiting.");
}

void ReindeerThread(int id)
{
    for(int round = 0; round < REINDEER_VACATION_ROUNDS; round++){
        utils::RandomSleep(500, 1000, SLOWDOWN_FACTOR);
        {
            std::unique_lock lock(mtx);
            reindeerCount++;
            utils::SafePrint("[Reindeer " + std::to_string(id) + "] Returned. ReindeerCount=" + std::to_string(reindeerCount));
            
            if(reindeerCount == NUM_REINDEER){
                doubleReindeerSem.acquire();
                utils::SafePrint("[Reindeer " + std::to_string(id) + "] I'm the last! Waking Santa!");
                santaSem.release();
                doubleReindeerSem.release();
            }
        }

            reindeerSem.acquire();

        utils::SafePrint("[Reindeer " + std::to_string(id) + "] Delivering toys..");
        utils::RandomSleep(300, 600, SLOWDOWN_FACTOR);
        utils::SafePrint("[Reindeer " + std::to_string(id) + "] Going back on vacation..");
    }
     utils::SafePrint("[Reindeer " + std::to_string(id) + "] Done, exicing thread.");

}

void santaThread(){

    utils::SafePrint("[Santa] Ho-ho-ho, I am here...");
    for(;;){
        doubleReindeerSem.release();
        santaSem.acquire();
        std::unique_lock lock(mtx);

        if(reindeerCount == NUM_REINDEER){
            utils::SafePrint("[Santa] All reindeer have arrived!Preparing the sleigh..");
            reindeerCount = 0;
            doubleReindeerSem.acquire();
            for(int i = 0; i<NUM_REINDEER; i++){
                utils::SafePrint("[Santa] Reindeer" + std::to_string(i) + " released!");
                reindeerSem.release();
            }
            
            lock.unlock();

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            utils::SafePrint("[Santa] Last Reindeer released!");
            
            utils::SafePrint("[Santa] Done delivering toys; back to sleep!");
        }
        else if (elfCount == 3)
        {
            // Handle elves as before (unchanged)
            utils::SafePrint("[Santa] 3 elves need help. Letting them in...");

            // Release the first 2 elves that were waiting on santaSignal
            for (int i = 0; i < (GROUP_SIZE - 1); i++) {
                utils::SafePrint("[Santa] 2 Elves released!");
                santaSignal.release();
            }

            // Reset the elfCount while locked
            elfCount = 0;

            // Let all 3 elves ask their questions
            for (int i = 0; i < GROUP_SIZE; i++) {
                utils::SafePrint("[Santa] 3 Elves problems solved!");
                problem.release();
            }

            // Help them (simulate)
            lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(700));
            lock.lock();

            utils::SafePrint("[Santa] Done helping these elves!");

            // Let the 3 elves know Santa’s done
            for (int i = 0; i < GROUP_SIZE; i++) {
                elfDone.release();
            }
        }
        else
        {
            // Handle spurious wakeups or partial groups (unchanged)
            utils::SafePrint("[Santa] Woke up, but ReindeerCount="
                      + std::to_string(reindeerCount) + ", ElfCount=" + std::to_string(elfCount));
        }
    }
}

int main(){

    srand((unsigned int)time(nullptr));
    std::thread santa(santaThread);

    std::vector<std::thread> reindeers;
    reindeers.reserve(NUM_REINDEER);
    for(int i=1; i<=NUM_REINDEER;++i){
        reindeers.emplace_back(ReindeerThread, i);
    }
    std::vector<std::thread> elves;
    elves.reserve(NUM_ELVES);
    for(int j=1; j<=NUM_ELVES;++j){
        elves.emplace_back(ElfThread, j);
    }


    for (auto &e : elves) {
        if(e.joinable()){ e.join();}

        else{ std::cout<< "not joinable" << '\n';}
    }
    for (auto &r : reindeers) {
         if(r.joinable()){ r.join();}

        else{ std::cout<< "not joinable" << '\n';}
    }

      santa.detach();
    utils::SafePrint("[Main] All reindeer and elves finished. Santa still dozing.");
}
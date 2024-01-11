#ifndef FRONTIER_POOL_H
#define FRONTIER_POOL_H

#include <memory>
#include <queue>
#include <mutex>
#include <condition_variable>
#include "Frontier.h"  // Include the Frontier class definition

template<typename IT>
class FrontierPool {
public:
    FrontierPool(IT graphN, IT graphM);
    std::shared_ptr<Frontier<IT>> getFrontier();
    void returnFrontier(std::shared_ptr<Frontier<IT>> frontier);
    static void generateFrontiers(FrontierPool<IT>& frontierPool, std::size_t numFrontiers);
private:
    IT graphN;
    IT graphM;
    std::queue<std::shared_ptr<Frontier<IT>>> frontiers;
    std::mutex mutex;
    std::condition_variable condition;
};

// Constructor
template<typename IT>
FrontierPool<IT>::FrontierPool(IT graphN, IT graphM)
    : graphN(graphN), graphM(graphM) {}

// Get a frontier from the pool
template<typename IT>
std::shared_ptr<Frontier<IT>> FrontierPool<IT>::getFrontier() {
    std::unique_lock<std::mutex> lock(mutex);

    // Wait until a frontier is available
    while (frontiers.empty()) {
        condition.wait(lock);
    }

    // Get a frontier from the pool
    auto frontier = frontiers.front();
    frontiers.pop();

    return frontier;
}

// Return a frontier to the pool
template<typename IT>
void FrontierPool<IT>::returnFrontier(std::shared_ptr<Frontier<IT>> frontier) {
    frontier->reinit();
    frontier->clear();
    
    std::unique_lock<std::mutex> lock(mutex);

    // Return the frontier to the pool
    frontiers.push(frontier);

    // Notify waiting threads that a frontier is available
    condition.notify_one();
}

// Static method definition
template<typename IT>
void FrontierPool<IT>::generateFrontiers(FrontierPool<IT>& frontierPool, std::size_t numFrontiers) {
    for (std::size_t i = 0; i < numFrontiers; ++i) {
        auto newFrontier = std::make_shared<Frontier<IT>>(frontierPool.graphN, frontierPool.graphM);
        frontierPool.returnFrontier(newFrontier);
    }
}

#endif // FRONTIER_POOL_H

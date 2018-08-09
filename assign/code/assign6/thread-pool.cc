/**
 * File: thread-pool.cc
 * --------------------
 * Presents the implementation of the ThreadPool class.
 */

#include <iostream>
#include "ostreamlock.h"
#include "thread-pool.h"
using namespace std;

ThreadPool::ThreadPool(size_t numThreads) : wts(numThreads), thq(numThreads) {
  dt = thread([this](){
    dispatcher();
  });

  for (size_t workerID = 0; workerID < numThreads; workerID++) {
    unique_ptr<semaphore>& s = thq[workerID].first;
    s.reset(new semaphore);
    wts[workerID] = thread([this](size_t workerID){
      worker(workerID);
    }, workerID);
    tq.push(workerID);
  }
}

void ThreadPool::dispatcher() {
  while (true) {
    // Consider if I should replace m + cv with sems
    jm.lock();
    jcv.wait(jm, [this]{ return allScheduled || !jq.empty(); });
    jm.unlock();

    sm.lock();
    if (allScheduled) {
      sm.unlock();
      for (auto& thqp: thq) thqp.first->signal();
      break;
    }
    sm.unlock();

    jm.lock();
    Thunk& t = jq.front();
    jq.pop();
    jcv.notify_all();
    jm.unlock();

    tm.lock();
    tcv.wait(tm, [this]{ return !tq.empty(); });
    size_t workerID = tq.front();
    tq.pop();
    tm.unlock();

    thq[workerID].second = t;
    thq[workerID].first->signal();
  }

  cout << oslock << "dispatcher done" << endl << osunlock;
}

void ThreadPool::worker(int workerID) {
  while (true) {
    thq[workerID].first->wait();

    sm.lock();
    if (allScheduled) {
      sm.unlock();
      break;
    }
    sm.unlock();
    thq[workerID].second();

    tm.lock();
    tq.push(workerID);
    tcv.notify_all();
    tm.unlock();
  }

  cout << oslock << "worker " << workerID << " done." << endl << osunlock;
}

void ThreadPool::schedule(const Thunk& thunk) {
  lock_guard<mutex> lg(jm);
  jq.push(thunk);
  jcv.notify_all();
}

void ThreadPool::wait() {
  jm.lock();
  jcv.wait(jm, [this]{ return jq.empty(); });
  jm.unlock();
  tm.lock();
  tcv.wait(tm, [this]{ return tq.size() == wts.size(); });
  tm.unlock();
  cout << "Done!" << endl;
}

ThreadPool::~ThreadPool() {
  sm.lock();
  allScheduled = true;
  jcv.notify_all();
  sm.unlock();

  dt.join();
  for (thread& t: wts) {
    cout << oslock << "joining" << endl << osunlock;
    t.join();
    cout << oslock << "joined" << endl << osunlock;
  }

//  for (auto& thqp : thq) {
//    thqp.first.release();
//  }
}

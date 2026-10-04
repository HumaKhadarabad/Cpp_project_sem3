#include "Simulator.h"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>
using namespace std;

namespace {
// Minimal reusable barrier (barrier needs C++20)   The last thread to arrive
// runs `onComplete` before everybody is released
class Barrier {
public:
    Barrier(size_t n, function<void()> onComplete = {})
        : n_(n), onComplete_(move(onComplete)) {}
    void wait() {
        unique_lock<mutex> lk(m_);
        auto gen = generation_;
        if (++waiting_ == n_) {
            waiting_ = 0;
            if (onComplete_) onComplete_();
            ++generation_;
            cv_.notify_all();
        } else {
            cv_.wait(lk, [&] { return gen != generation_; });
        }
    }
private:
    mutex m_;
    condition_variable cv_;
    size_t n_, waiting_ = 0, generation_ = 0;
    function<void()> onComplete_;
};
}


Simulator::Simulator(Component& top, size_t threads)
    : threads_(max<size_t>(1, threads)) {
    top.collectGates(gates_);
    if (gates_.empty()) throw invalid_argument("circuit contains no gates");
    threads_ = min(threads_, gates_.size());
}

void Simulator::addProbe(const string& label, Wire* w) { addBus(label, {w}); }

void Simulator::addBus(const string& label, const vector<Wire*>& msbFirst) {
    probes_.push_back({label, msbFirst});
    vector<string> names;
    for (auto& p : probes_) names.push_back(p.name);
    result_.setColumns(names);
}

int Simulator::settle() {
    return threads_ <= 1 ? settleSequential() : settleParallel();
}

int Simulator::settleSequential() {
    for (int round = 1; round <= kMaxRounds; ++round) {
        for (Gate* g : gates_) g->evaluate();
        bool changed = false;
        for (Gate* g : gates_) changed |= g->commit();
        if (!changed) return round;
    }
    throw runtime_error("circuit did not stabilise (oscillation?)");
}

int Simulator::settleParallel() {
    const size_t T = threads_, n = gates_.size();
    atomic<int> changes{0};
    bool done = false, failed = false;
    int rounds = 0;

    Barrier afterEvaluate(T);
    Barrier afterCommit(T, [&] {           // runs in exactly one thread
        ++rounds;
        if (changes.load() == 0) done = true;
        else if (rounds >= kMaxRounds) { done = true; failed = true; }
        changes = 0;
    });

    auto worker = [&](size_t id) {
        const size_t lo = n * id / T, hi = n * (id + 1) / T;
        while (true) {
            for (size_t i = lo; i < hi; ++i) gates_[i]->evaluate();   // phase 1
            afterEvaluate.wait();
            int local = 0;
            for (size_t i = lo; i < hi; ++i) local += gates_[i]->commit();  // phase 2
            changes += local;
            afterCommit.wait();
            if (done) break;
        }
    };

    vector<thread> pool;
    for (size_t t = 0; t < T; ++t) pool.emplace_back(worker, t);
    for (auto& th : pool) th.join();

    if (failed) throw runtime_error("circuit did not stabilise (oscillation?)");
    return rounds;
}

void Simulator::record() {
    vector<long> vals;
    for (auto& p : probes_) {
        long v = 0;
        for (Wire* w : p.wires) v = v * 2 + (w->get() ? 1 : 0);
        vals.push_back(v);
    }
    result_.addRow(time_++, move(vals));
}

void Simulator::tickClock(Wire* clk) {
    set(clk, true);  settle(); record();
    set(clk, false); settle(); record();
}

vector<size_t> Simulator::threadLoad() const {
    vector<size_t> load;
    const size_t T = threads_, n = gates_.size();
    for (size_t id = 0; id < T; ++id) load.push_back(n * (id + 1) / T - n * id / T);
    return load;
}

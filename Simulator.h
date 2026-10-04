// Simulator.h - drives a circuit: sets inputs, lets signals settle, records probes

// threads == 1 : all gates evaluated one after another
// threads  > 1 : STRETCH GOAL: Gates are split between worker threads; in every
//                round all threads evaluate their gates concurrently, wait at a
//                barrier, commit, wait again. One round = one gate delay
#pragma once
#include "Component.h"
#include "SimulationResult.h"
#include <string>
#include <vector>
using namespace std;


class Simulator {
public:
    explicit Simulator(Component& top, size_t threads = 1);

    // Probes (columns of the result). A bus is listed MSB first and is stored as an integer.
    void addProbe(const string& label, Wire* w);
    void addBus(const string& label, const vector<Wire*>& msbFirst);

    void set(Wire* w, bool v) { w->set(v); }   // drive a primary input
    int  settle();                             // propagate until stable, returns #rounds
    void record();                             // append one row (time auto-increments)
    void tickClock(Wire* clk);                 // clk=1 settle record, clk=0 settle record

    const SimulationResult& result() const { return result_; }
    size_t gateCount() const { return gates_.size(); }
    size_t threads() const { return threads_; }
    vector<size_t> threadLoad() const;   // gates handled by each thread

private:
    int settleSequential();
    int settleParallel();

    struct Probe { string name; vector<Wire*> wires; };
    vector<Gate*> gates_;
    vector<Probe> probes_;
    size_t threads_;
    long time_ = 0;
    SimulationResult result_;
    static constexpr int kMaxRounds = 5000;
};

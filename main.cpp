// main.cpp - demonstrations. Usage: ./circuit_designer [threads]   (default 4)
// Produces gates.csv, adder.csv, counter.csv (open in Excel to plot)
#include "Circuits.h"
#include "Output.h"
#include "Simulator.h"
#include <iostream>
#include <memory>
#include <string>
using namespace std;

static void report(const Simulator& s) {
    cout << "  [" << s.gateCount() << " NAND/NOR gates, " << s.threads() << " thread(s); gates per thread:";
    for (auto n : s.threadLoad()) cout << ' ' << n;
    cout << "]\n";
}

static void demoGates(size_t threads) {
    cout << "\n=== 1. Gates built from NAND / NOR ===\n";
    Wire a("A"), b("B");
    // one big circuit that contains all gates so a single Simulator runs them together
    struct All : Circuit {
        Wire *na, *and_, *or_, *xor_, *xnor_, *and_n, *or_n, *xor_n;
        All(Wire* a, Wire* b) : Circuit("all") {
            na    = add<NotGate>("not", a)->out();
            and_  = add<AndGate>("and", a, b)->out();
            or_   = add<OrGate>("or", a, b)->out();
            xor_  = add<XorGate>("xor", a, b)->out();
            xnor_ = add<XnorGate>("xnor", a, b)->out();
            and_n = add<AndNorGate>("and_nor", a, b)->out();
            or_n  = add<OrNorGate>("or_nor", a, b)->out();
            xor_n = add<XorNorGate>("xor_nor", a, b)->out();
        }
    } all(&a, &b);

    Simulator sim(all, threads);
    sim.addProbe("A", &a);           sim.addProbe("B", &b);
    sim.addProbe("NOT_A", all.na);
    sim.addProbe("AND", all.and_);   sim.addProbe("OR", all.or_);
    sim.addProbe("XOR", all.xor_);   sim.addProbe("XNOR", all.xnor_);
    sim.addProbe("AND_NOR", all.and_n); sim.addProbe("OR_NOR", all.or_n); sim.addProbe("XOR_NOR", all.xor_n);
    for (int i = 0; i < 4; ++i) {
        sim.set(&a, i & 2); sim.set(&b, i & 1);
        sim.settle(); sim.record();
    }
    report(sim);
    Output().write(sim.result());
    CSVOutput("gates.csv").write(sim.result());
    cout << "  -> gates.csv written\n";
}

static void demoAdder(size_t threads) {
    cout << "\n=== 2. 4-bit ripple-carry adder (all 512 input combinations) ===\n";
    vector<unique_ptr<Wire>> store;
    vector<Wire*> A, B;
    for (int i = 0; i < 4; ++i) {
        store.push_back(make_unique<Wire>("a" + to_string(i))); A.push_back(store.back().get());
        store.push_back(make_unique<Wire>("b" + to_string(i))); B.push_back(store.back().get());
    }
    Wire cin("cin");
    RippleCarryAdder add("adder4", A, B, &cin);

    Simulator sim(add, threads);
    sim.addBus("A", {A[3], A[2], A[1], A[0]});
    sim.addBus("B", {B[3], B[2], B[1], B[0]});
    sim.addProbe("cin", &cin);
    const auto& s = add.sum();
    sim.addBus("SUM", {add.cout(), s[3], s[2], s[1], s[0]});   // 5-bit result incl. carry out

    int bad = 0;
    for (int c = 0; c < 2; ++c)
        for (int x = 0; x < 16; ++x)
            for (int y = 0; y < 16; ++y) {
                for (int i = 0; i < 4; ++i) { sim.set(A[i], (x >> i) & 1); sim.set(B[i], (y >> i) & 1); }
                sim.set(&cin, c);
                sim.settle(); sim.record();
                long got = sim.result().rows().back().values.back();
                if (got != x + y + c) ++bad;
            }
    report(sim);
    CSVOutput("adder.csv").write(sim.result());
    cout << "  512 vectors simulated, " << bad << " wrong  -> adder.csv written\n";
    cout << "  example: 9 + 7 = " << 16 << " is in adder.csv (A=9,B=7,cin=0,SUM=16)\n";
}

static void demoCounter(size_t threads) {
    cout << "\n=== 3. 4-bit synchronous counter (20 clock cycles) ===\n";
    Wire clk("clk");
    Counter4 cnt("counter", &clk);
    Simulator sim(cnt, threads);
    sim.addProbe("clk", &clk);
    sim.addProbe("Q3", cnt.q(3)); sim.addProbe("Q2", cnt.q(2));
    sim.addProbe("Q1", cnt.q(1)); sim.addProbe("Q0", cnt.q(0));
    sim.addBus("COUNT", {cnt.q(3), cnt.q(2), cnt.q(1), cnt.q(0)});
    sim.settle(); sim.record();                  // power-on state
    for (int i = 0; i < 20; ++i) sim.tickClock(&clk);
    report(sim);
    Output().write(sim.result());
    CSVOutput("counter.csv").write(sim.result());
    cout << "  -> counter.csv written\n";
}

int main(int argc, char** argv) {
    size_t threads = 4;
    if (argc > 1) threads = max(1, atoi(argv[1]));
    cout << "Digital Electronics Circuit Designer  (threads = " << threads << ")\n";
    try {
        demoGates(threads);
        demoAdder(threads);
        demoCounter(threads);
    } catch (const exception& e) {
        cerr << "error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}

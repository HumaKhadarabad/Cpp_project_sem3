// tests.cpp - automatic verification. Exit code 0 = everything passed
#include "Circuits.h"
#include "Simulator.h"
#include <functional>
#include <iostream>
#include <memory>
using namespace std;

static int failures = 0, checks = 0;
#define CHECK(cond, msg) do { ++checks; if (!(cond)) { ++failures; cout << "  FAIL: " << msg << '\n'; } } while (0)


template <class G>
static void gateTest(const string& name, function<bool(bool, bool)> expect) {
    Wire a("a"), b("b");
    G g(name, &a, &b);
    Simulator sim(g);
    for (int i = 0; i < 4; ++i) {
        a.set(i & 2); b.set(i & 1); sim.settle();
        CHECK(g.out()->get() == expect(i & 2, i & 1), name << " inputs " << (i >> 1) << (i & 1));
    }
    cout << "  " << name << " (" << g.gateCount() << " primitive gates) checked\n";
}

static void testGates() {
    cout << "[gates]\n";
    gateTest<AndGate>("AND (NAND)",   [](bool a, bool b) { return a && b; });
    gateTest<OrGate>("OR (NAND)",     [](bool a, bool b) { return a || b; });
    gateTest<XorGate>("XOR (NAND)",   [](bool a, bool b) { return a != b; });
    gateTest<XnorGate>("XNOR (NAND)", [](bool a, bool b) { return a == b; });
    gateTest<AndNorGate>("AND (NOR)", [](bool a, bool b) { return a && b; });
    gateTest<OrNorGate>("OR (NOR)",   [](bool a, bool b) { return a || b; });
    gateTest<XorNorGate>("XOR (NOR)", [](bool a, bool b) { return a != b; });
    Wire a("a"); NotGate n("NOT", &a); Simulator s(n);
    for (int v = 0; v < 2; ++v) { a.set(v); s.settle(); CHECK(n.out()->get() == !v, "NOT " << v); }
    Wire c("c"), d("d");
    NandGate nd("nand", &c, &d); NorGate nr("nor", &c, &d);
    Simulator s1(nd), s2(nr);
    for (int i = 0; i < 4; ++i) {
        c.set(i & 2); d.set(i & 1); s1.settle(); s2.settle();
        CHECK(nd.out()->get() == !((i & 2) && (i & 1)), "NAND " << i);
        CHECK(nr.out()->get() == !((i & 2) || (i & 1)), "NOR " << i);
    }
}

static void testAdder(size_t threads) {
    cout << "[4-bit adder, " << threads << " thread(s)]\n";
    vector<unique_ptr<Wire>> st; vector<Wire*> A, B;
    for (int i = 0; i < 4; ++i) {
        st.push_back(make_unique<Wire>()); A.push_back(st.back().get());
        st.push_back(make_unique<Wire>()); B.push_back(st.back().get());
    }
    Wire cin;
    RippleCarryAdder ad("add", A, B, &cin);
    Simulator sim(ad, threads);
    int bad = 0;
    for (int c = 0; c < 2; ++c) for (int x = 0; x < 16; ++x) for (int y = 0; y < 16; ++y) {
        for (int i = 0; i < 4; ++i) { A[i]->set((x >> i) & 1); B[i]->set((y >> i) & 1); }
        cin.set(c); sim.settle();
        int got = ad.cout()->get() << 4;
        for (int i = 0; i < 4; ++i) got |= ad.sum()[i]->get() << i;
        if (got != x + y + c) ++bad;
    }
    CHECK(bad == 0, bad << " wrong sums out of 512");
    cout << "  512 combinations, " << bad << " wrong\n";
}

static void testLatch() {
    cout << "[D latch memory]\n";
    Wire d("d"), en("en");
    DLatch l("latch", &d, &en);
    Simulator sim(l);
    sim.settle();
    CHECK(!l.q()->get(), "power-on q=0");
    d.set(1); en.set(1); sim.settle(); CHECK(l.q()->get(), "transparent: q follows d=1");
    en.set(0); sim.settle();          CHECK(l.q()->get(), "holds 1 when en=0");
    d.set(0); sim.settle();           CHECK(l.q()->get(), "still holds 1 while d changes");
    en.set(1); sim.settle();          CHECK(!l.q()->get(), "transparent again: q=0");
}

static SimulationResult runCounter(size_t threads, int cycles, bool& ok) {
    Wire clk("clk");
    Counter4 c("cnt", &clk);
    Simulator sim(c, threads);
    sim.addProbe("clk", &clk);
    sim.addBus("COUNT", {c.q(3), c.q(2), c.q(1), c.q(0)});
    sim.settle(); sim.record();
    ok = true;
    for (int i = 1; i <= cycles; ++i) {
        sim.tickClock(&clk);
        const auto& rows = sim.result().rows();
        long high = rows[rows.size() - 2].values[1], low = rows.back().values[1];
        if (high != i % 16 || low != i % 16) ok = false;
    }
    return sim.result();
}

static void testCounter() {
    cout << "[4-bit counter]\n";
    bool ok1, ok4;
    auto r1 = runCounter(1, 40, ok1);
    auto r4 = runCounter(4, 40, ok4);
    CHECK(ok1, "sequential counter counts 1,2,...,15,0,1,... for 40 clocks");
    CHECK(ok4, "4-thread counter counts correctly");
    CHECK(r1 == r4, "multi-threaded result identical to single-threaded result");
    cout << "  40 clock cycles, wrap-around at 16, threaded == sequential: "
              << (r1 == r4 ? "yes" : "NO") << "\n";
}

int main() {
    try {
        testGates();
        testAdder(1);
        testAdder(4);
        testLatch();
        testCounter();
    } catch (const exception& e) {
        cout << "EXCEPTION: " << e.what() << '\n';
        return 2;
    }
    cout << "\n" << (checks - failures) << "/" << checks << " checks passed\n";
    cout << (failures ? "RESULT: FAILED\n" : "RESULT: ALL TESTS PASSED\n");
    return failures ? 1 : 0;
}

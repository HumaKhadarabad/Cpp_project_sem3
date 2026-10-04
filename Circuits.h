// Circuits.h - complex circuits built by connecting gates / smaller circuits

//   HalfAdder, FullAdder, RippleCarryAdder (4-bit in the demo),
//   DLatch, DFlipFlop (master/slave), Buffer, Counter4 (synchronous 4-bit counter)
// All wiring is done inside the constructors.
#pragma once
#include "Gates.h"
#include <stdexcept>
#include <vector>
using namespace std;


// ------------------------------- adders -----------------------------------
class HalfAdder : public Circuit {
public:
    HalfAdder(const string& name, Wire* a, Wire* b) : Circuit(name) {
        sum_   = add<XorGate>(name + ".xor", a, b)->out();
        carry_ = add<AndGate>(name + ".and", a, b)->out();
    }
    Wire* sum() const { return sum_; }
    Wire* carry() const { return carry_; }
private:
    Wire *sum_, *carry_;
};

class FullAdder : public Circuit {
public:
    FullAdder(const string& name, Wire* a, Wire* b, Wire* cin) : Circuit(name) {
        auto* h1 = add<HalfAdder>(name + ".h1", a, b);
        auto* h2 = add<HalfAdder>(name + ".h2", h1->sum(), cin);
        auto* orc = add<OrGate>(name + ".or", h1->carry(), h2->carry());
        sum_  = h2->sum();
        cout_ = orc->out();
    }
    Wire* sum() const { return sum_; }
    Wire* cout() const { return cout_; }
private:
    Wire *sum_, *cout_;
};

// N-bit adder; vectors are LSB first (a[0] is bit 0). Demo uses N = 4.
class RippleCarryAdder : public Circuit {
public:
    RippleCarryAdder(const string& name, const vector<Wire*>& a,
                     const vector<Wire*>& b, Wire* cin) : Circuit(name) {
        if (a.size() != b.size() || a.empty())
            throw invalid_argument("adder inputs must have the same non-zero size");
        Wire* carry = cin;
        for (size_t i = 0; i < a.size(); ++i) {
            auto* fa = add<FullAdder>(name + ".fa" + to_string(i), a[i], b[i], carry);
            sum_.push_back(fa->sum());
            carry = fa->cout();
        }
        cout_ = carry;
    }
    const vector<Wire*>& sum() const { return sum_; }   // LSB first
    Wire* cout() const { return cout_; }
private:
    vector<Wire*> sum_;
    Wire* cout_;
};


// --------------------------- memory elements ------------------------------
// Gated D latch from 5 NANDs. Transparent while `en` = 1, holds while en = 0.
// Power-on state is q = 0.
class DLatch : public Circuit {
public:
    DLatch(const string& name, Wire* d, Wire* en) : Circuit(name) {
        auto* nd = add<NotGate>(name + ".nd", d);
        auto* s  = add<NandGate>(name + ".s", d, en);
        auto* r  = add<NandGate>(name + ".r", nd->out(), en);
        q_  = newWire(name + ".q", false);    // feedback wires are created first...
        qn_ = newWire(name + ".qn", true);
        add<NandGate>(name + ".q", s->out(), qn_, q_);      // ...then driven by the
        add<NandGate>(name + ".qn", r->out(), q_, qn_);     // cross-coupled NANDs
    }
    Wire* q() const { return q_; }
    Wire* qn() const { return qn_; }
private:
    Wire *q_, *qn_;
};

// Master/slave D flip-flop: output changes on the RISING edge of clk.
class DFlipFlop : public Circuit {
public:
    DFlipFlop(const string& name, Wire* d, Wire* clk) : Circuit(name) {
        auto* nclk   = add<NotGate>(name + ".nclk", clk);
        auto* master = add<DLatch>(name + ".master", d, nclk->out());
        auto* slave  = add<DLatch>(name + ".slave", master->q(), clk);
        q_ = slave->q();
    }
    Wire* q() const { return q_; }
private:
    Wire* q_;
};

// Two inverters in series that drive an already existing wire.
class Buffer : public Circuit {
public:
    Buffer(const string& name, Wire* in, Wire* out) : Circuit(name) {
        auto* n1 = add<NotGate>(name + ".n1", in);
        add<NotGate>(name + ".n2", n1->out(), out);
    }
};


// ------------------------------ counter -----------------------------------
// Synchronous 4-bit binary counter: 4 D flip-flops + an incrementer made of
// a NOT and three half adders. Counts 0..15 and wraps, +1 per rising clock edge.
class Counter4 : public Circuit {
public:
    Counter4(const string& name, Wire* clk) : Circuit(name) {
        for (int i = 0; i < 4; ++i) {
            d_[i] = newWire(name + ".d" + to_string(i));
            q_[i] = add<DFlipFlop>(name + ".ff" + to_string(i), d_[i], clk)->q();
        }
        // next = q + 1
        auto* n0 = add<NotGate>(name + ".inc.not0", q_[0]);
        add<Buffer>(name + ".buf0", n0->out(), d_[0]);
        Wire* carry = q_[0];
        for (int i = 1; i < 4; ++i) {
            auto* ha = add<HalfAdder>(name + ".inc.ha" + to_string(i), q_[i], carry);
            add<Buffer>(name + ".buf" + to_string(i), ha->sum(), d_[i]);
            carry = ha->carry();
        }
    }
    Wire* q(int bit) const { return q_[bit]; }   // bit 0 = LSB
private:
    Wire* d_[4];
    Wire* q_[4];
};

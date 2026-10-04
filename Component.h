// Component.h - base classes of the whole design

//   Component  : anything that can be part of a circuit
//   Gate       : a primitive 2-input gate (only NAND / NOR derive from it)
//   Circuit    : a component built by connecting other components

// Simulation model: unit-delay, two-phase
//   Phase 1 (evaluate): every gate computes its next output from the CURRENT wires
//   Phase 2 (commit)  : every gate writes its output wire
// Because phase 1 only reads wires and phase 2 only writes them, all gates can
// run concurrently on different threads without locks (see Simulator.cpp)
#pragma once
#include "Wire.h"
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>
using namespace std;

class Gate;

// Component Class
class Component {
public:
    explicit Component(string name) : name_(move(name)) {}
    virtual ~Component() = default;
    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;

    const string& name() const { return name_; }

    // Flatten this component into the primitive gates it is made of.
    virtual void collectGates(vector<Gate*>& gates) = 0;

    size_t gateCount() {
        vector<Gate*> g;
        collectGates(g);
        return g.size();
    }

private:
    string name_;
};

// Gate Class
class Gate : public Component {
public:
    // If `out` is null the gate creates (and owns) its own output wire.
    Gate(string name, Wire* a, Wire* b, Wire* out = nullptr)
        : Component(move(name)), a_(a), b_(b) {
        if (out) {
            out_ = out;
        } else {
            owned_ = make_unique<Wire>(this->name() + ".out");
            out_ = owned_.get();
        }
    }

    virtual bool compute() const = 0;   // the truth table of the gate

    void evaluate() { next_ = compute(); }          // phase 1
    bool commit() {                                 // phase 2, true if output changed
        if (out_->get() == next_) return false;
        out_->set(next_);
        return true;
    }

    Wire* out() const { return out_; }
    void collectGates(vector<Gate*>& gates) override { gates.push_back(this); }

protected:
    // Called by the concrete gate's constructor so the gate starts in a
    // consistent state instead of glitching at power-on.
    void initOutput() { out_->set(compute()); }

    Wire* a_;
    Wire* b_;
    Wire* out_ = nullptr;

private:
    unique_ptr<Wire> owned_;
    bool next_ = false;
};

// Circuit Class
// A Circuit owns its sub-components and internal wires. Subclasses do the
// wiring inside their constructor using add<>() and newWire().
class Circuit : public Component {
public:
    explicit Circuit(string name) : Component(move(name)) {}

    void collectGates(vector<Gate*>& gates) override {
        for (auto& p : parts_) p->collectGates(gates);
    }

protected:
    template <class T, class... Args>
    T* add(Args&&... args) {
        auto p = make_unique<T>(forward<Args>(args)...);
        T* raw = p.get();
        parts_.push_back(move(p));
        return raw;
    }

    Wire* newWire(const string& name, bool initial = false) {
        wires_.push_back(make_unique<Wire>(name, initial));
        return wires_.back().get();
    }

private:
    vector<unique_ptr<Component>> parts_;
    vector<unique_ptr<Wire>> wires_;
};

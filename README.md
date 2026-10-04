# Digital Electronics Circuit Designer

A C++ object-oriented simulator in which **every logic gate and digital circuit is built from just two primitive gates: NAND and NOR**. Gates are combined into larger circuits (adders, latches, flip-flops, a 4-bit counter), simulated with chosen inputs, and the readings are exported as **CSV files for plotting in Excel**. The simulator can also run all gates **concurrently on multiple threads**.

This project was built for the C++ OOP course (project idea: *Digital Electronics Circuit Designer*).

---

## Table of contents
1. [Team](#team)
2. [Features](#features)
3. [How it works](#how-it-works)
4. [Repository structure](#repository-structure)
5. [Requirements](#requirements)
6. [Build and run](#build-and-run)
7. [Usage](#usage)
8. [Output files](#output-files)
9. [Testing and verification](#testing-and-verification)
10. [OOP concepts used](#oop-concepts-used)


---

## Team

| Member | Files | Responsibility |
|---|---|---|
| **Yuktha** | `Output.h`, `Output.cpp`, `main.cpp`, `tests.cpp` | Console and CSV output, demonstrations, automatic tests |
| **Rithvik Lanka** | `SimulationResult.h`, `Simulator.h`, `Simulator.cpp`, `Component.h` | Simulation engine (single and multi-threaded), result table, component base classes |
| **Huma** | `Wire.h`, `Gates.h`, `Circuits.h` | Wires, NAND/NOR and derived gates, adders, latch, flip-flop, counter |

---

## Features

- **Primitive gates:** NAND and NOR are the only classes that compute anything.
- **Derived gates:** NOT, AND, OR, XOR, XNOR built from NAND; NOT, AND, OR, XOR built from NOR.
- **Complex circuits:** half adder, full adder, 4-bit ripple-carry adder, D latch, master/slave D flip-flop, 4-bit synchronous counter.
- **Wiring during construction:** a circuit connects its own parts inside its constructor.
- **Simulator** with probes and buses that record readings over time.
- **Output classes:** `Output` (console table) and its subclass `CSVOutput` (CSV file for Excel).
- **Stretch goal:** multi-threaded simulation (`./circuit_designer <threads>`), verified to give identical results to the single-threaded run.
- **Automatic test suite** (48 checks).

---

## How it works

**Workflow:** build a circuit object, hand it to the `Simulator`, drive inputs, let the signals settle, record the readings, and write them out.

```
main -> builds Circuit -> Simulator(circuit) -> set / settle / record
                                                      |
                                              SimulationResult
                                                      |
                                          Output  /  CSVOutput (.csv)
```

**Composition hierarchy** (what is built from what):

```
Counter4
 |- 4 x DFlipFlop  = NOT + 2 x DLatch (5 NAND gates each)
 |- incrementer    = NOT + 3 x HalfAdder (XOR + AND)
 |- 4 x Buffer     = 2 x NOT
RippleCarryAdder -> FullAdder -> 2 x HalfAdder + OrGate -> XOR / AND / OR -> NAND
```

**Simulation model (two-phase, unit delay).** Each simulation round represents one gate delay:

1. **Evaluate:** every gate reads the current wire values and computes its next output.
2. **Commit:** every gate writes its output wire.

Rounds repeat until no wire changes (the circuit is *stable*). Because phase 1 only reads wires and phase 2 only writes them, all gates can run on different threads without locks. Two barriers separate the phases. This model also handles feedback naturally, which is how latches and flip-flops work.

---

## Repository structure

```
.
├── Wire.h                 # a single 0/1 signal connecting components
├── Component.h            # Component, Gate and Circuit base classes
├── Gates.h                # NAND, NOR and every gate built from them
├── Circuits.h             # adders, D latch, D flip-flop, Buffer, 4-bit counter
├── SimulationResult.h     # table of recorded readings
├── Simulator.h / .cpp     # simulation engine, single and multi-threaded
├── Output.h / .cpp        # Output (console) and CSVOutput (CSV file)
├── main.cpp               # demonstrations (writes the CSV files)
├── tests.cpp              # automatic verification
├── Makefile               # build, run and test targets
├── README.md              # this file
├── REPORT.md              # project report / documentation
└── class_diagram.mermaid  # UML class diagram
```

---

## Requirements

- A C++17 compiler (**g++ 7 or newer** or clang++)
- `make` (optional, you can compile by hand)
- POSIX threads (included with g++ on Linux/macOS; `-pthread` flag)
- Excel or any spreadsheet program to plot the CSV files (optional)

**Windows:** use MinGW-w64 g++ or WSL (Windows Subsystem for Linux) with the same commands.

---

## Build and run

```bash
make              # builds circuit_designer and tests
make test         # runs the automatic verification
make run          # runs the demo with 4 threads
make clean        # removes binaries and generated CSV files
```

Without `make`:

```bash
g++ -std=c++17 -O2 -pthread main.cpp Simulator.cpp Output.cpp -o circuit_designer
g++ -std=c++17 -O2 -pthread tests.cpp Simulator.cpp Output.cpp -o tests
```

---

## Usage

```bash
./circuit_designer          # demo with the default 4 threads
./circuit_designer 1        # single-threaded (no worker threads)
./circuit_designer 8        # 8 worker threads
```

The program prints a short report for each demonstration, including how many NAND/NOR gates the circuit contains and how many gates each thread handles, then writes the CSV files into the current directory.

The three demonstrations are:

| # | Demonstration | What it shows |
|---|---|---|
| 1 | Gates from NAND / NOR | Truth tables of NOT, AND, OR, XOR, XNOR (NAND-based) and AND, OR, XOR (NOR-based) for all four input combinations |
| 2 | 4-bit adder | All 512 input combinations (A, B, carry-in) with the 5-bit result; every sum is checked against `A + B + cin` |
| 3 | 4-bit counter | 20 clock cycles, counting 0 to 15 and wrapping back to 0 |

### Writing your own circuit

```cpp
#include "Circuits.h"
#include "Output.h"
#include "Simulator.h"

int main() {
    Wire a("A"), b("B");
    XorGate x("xor", &a, &b);            // built from 4 NAND gates

    Simulator sim(x, 1);                 // 1 = single thread, N = N threads
    sim.addProbe("A", &a);
    sim.addProbe("B", &b);
    sim.addProbe("XOR", x.out());

    for (int i = 0; i < 4; ++i) {
        sim.set(&a, i & 2);
        sim.set(&b, i & 1);
        sim.settle();                    // let the signals propagate
        sim.record();                    // store one row
    }
    Output().write(sim.result());                  // console table
    CSVOutput("my_xor.csv").write(sim.result());   // CSV file
}
```

Compile it with `g++ -std=c++17 -pthread your_file.cpp Simulator.cpp Output.cpp`.

---

## Output files

Running the demo creates three CSV files. Each has a header row followed by one row per recorded time step.

| File | Contents |
|---|---|
| `gates.csv` | `time, A, B, NOT_A, AND, OR, XOR, XNOR, AND_NOR, OR_NOR, XOR_NOR` |
| `adder.csv` | `time, A, B, cin, SUM` (SUM is the 5-bit result including carry-out) |
| `counter.csv` | `time, clk, Q3, Q2, Q1, Q0, COUNT` |

Example, the first rows of `counter.csv`:

```
time,clk,Q3,Q2,Q1,Q0,COUNT
0,0,0,0,0,0,0
1,1,0,0,0,1,1
2,0,0,0,0,1,1
3,1,0,0,1,0,2
```

**Plotting in Excel:** open the CSV, select the columns, then *Insert → Line chart*. Plotting `clk` and `COUNT` from `counter.csv` gives a timing diagram of the counter.

---

## Testing and verification

```bash
make test
```

The test program (`tests.cpp`) runs 48 automatic checks and exits with code 0 if all pass:

1. Truth tables of every gate (NAND, NOR, NOT, AND, OR, XOR, XNOR and the NOR-based versions).
2. The 4-bit adder against `A + B + cin` for all 512 input combinations, with 1 thread and with 4 threads.
3. The D latch remembers its value when enable is off, and follows its input when enable is on.
4. The 4-bit counter over 40 clock cycles, including wrap-around after 15.
5. The multi-threaded counter output is identical to the single-threaded output.

A successful run ends with:

```
RESULT: ALL TESTS PASSED
```

---

## OOP concepts used

| Concept | Where |
|---|---|
| **Abstraction** | `Component` and `Gate` are abstract (`collectGates`, `compute` are pure virtual) |
| **Inheritance** | `Gate` to `NandGate`/`NorGate`; `Circuit` to `LogicBlock` to `AndGate` etc.; `Circuit` to `FullAdder`, `Counter4`; `Output` to `CSVOutput` |
| **Polymorphism** | The `Simulator` calls `compute()` through `Gate*`; `Output::write` is overridden by `CSVOutput` |
| **Encapsulation** | Private wire values; each circuit exposes only its ports (`sum()`, `cout()`, `q()`) |
| **Composition** | A `Circuit` owns its parts and internal wires through `unique_ptr` |
| **Templates** | `Circuit::add<T>(args...)` creates and stores any component type |
| **RAII / memory safety** | Smart pointers only (no `new` / `delete`); copying of wires and components is disabled |
| **Concurrency** | `std::thread` with a custom barrier in `Simulator::settleParallel()` |

---


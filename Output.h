// Output.h - reads a SimulationResult and presents it
//   Output     : pretty table on the console
//   CSVOutput  : subclass that dumps the readings to a CSV file (open in Excel)
#pragma once
#include "SimulationResult.h"
#include <iostream>
#include <string>
using namespace std;


class Output {
public:
    explicit Output(ostream& os = cout) : os_(os) {}
    virtual ~Output() = default;
    virtual void write(const SimulationResult& result);   // console table
protected:
    ostream& os_;
};

class CSVOutput : public Output {
public:
    explicit CSVOutput(string path) : path_(move(path)) {}
    void write(const SimulationResult& result) override;  // writes the file
    const string& path() const { return path_; }
private:
    string path_;
};

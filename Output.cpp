#include "Output.h"
#include <fstream>
#include <iomanip>
#include <stdexcept>
using namespace std;

void Output::write(const SimulationResult& r) {
    const auto& cols = r.columns();
    os_ << setw(6) << "time";
    for (const auto& c : cols) os_ << ' ' << setw(max<size_t>(5, c.size())) << c;
    os_ << '\n';
    for (const auto& row : r.rows()) {
        os_ << setw(6) << row.time;
        for (size_t i = 0; i < row.values.size(); ++i)
            os_ << ' ' << setw(max<size_t>(5, cols[i].size())) << row.values[i];
        os_ << '\n';
    }
}

void CSVOutput::write(const SimulationResult& r) {
    ofstream f(path_);
    if (!f) throw runtime_error("cannot open " + path_ + " for writing");
    f << "time";
    for (const auto& c : r.columns()) f << ',' << c;
    f << '\n';
    for (const auto& row : r.rows()) {
        f << row.time;
        for (long v : row.values) f << ',' << v;
        f << '\n';
    }
}

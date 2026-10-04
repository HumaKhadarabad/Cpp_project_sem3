// SimulationResult.h - a table of recorded readings (one column per probe)
#pragma once
#include <string>
#include <vector>
using namespace std;

class SimulationResult {
public:
    struct Row {
        long time;
        vector<long> values;
    };

    void setColumns(vector<string> names) { names_ = move(names); }
    void addRow(long time, vector<long> values) { rows_.push_back({time, move(values)}); }
    void clear() { rows_.clear(); }

    const vector<string>& columns() const { return names_; }
    const vector<Row>& rows() const { return rows_; }

    bool operator==(const SimulationResult& o) const {
        if (names_ != o.names_ || rows_.size() != o.rows_.size()) return false;
        for (size_t i = 0; i < rows_.size(); ++i)
            if (rows_[i].time != o.rows_[i].time || rows_[i].values != o.rows_[i].values) return false;
        return true;
    }

private:
    vector<string> names_;
    vector<Row> rows_;
};

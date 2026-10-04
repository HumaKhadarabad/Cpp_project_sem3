// Wire.h - a single signal (0/1) that connects components
#pragma once
#include <string>
#include <utility>
using namespace std;


class Wire {
public:
    explicit Wire(string name = "", bool initial = false)
        : name_(move(name)), value_(initial) {}

    Wire(const Wire&) = delete;
    Wire& operator=(const Wire&) = delete;

    bool get() const { return value_; }
    void set(bool v) { value_ = v; }
    const string& name() const { return name_; }

private:
    string name_;
    bool value_;
};

/*
 * Test cases for evaluating the performance of binary search trees
 *
 * Copyright (c) 2016 Christopher Haster
 * Distributed under the MIT license
 */

#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <random>
#include <cassert>
#include <cmath>

// Test classes
#include <map>
#include <unordered_map>
#include "trees/utree.hpp"
#include "trees/compact_utree.hpp"
#include "trees/linear_utree.hpp"

#ifndef TEST_SIZE
#define TEST_SIZE 16384
#endif

#ifndef TEST_RUNS
#define TEST_RUNS 5
#endif

#ifndef TEST_RUNTIME
#define TEST_RUNTIME true
#endif

#ifndef TEST_INSTRUCTIONS
#define TEST_INSTRUCTIONS true
#endif

#ifndef TEST_HEAP
#define TEST_HEAP true
#endif

#ifndef TEST_CASES
#define TEST_CASES                  \
    test_case(lookups_test);        \
    test_case(insertions_test);     \
    test_case(deletions_test);      \
    test_case(iteration_test);
#endif

#ifndef TEST_CLASSES
#define TEST_CLASSES                \
    test_class(std::map);           \
    test_class(std::unordered_map); \
    test_class(utree);              \
    test_class(compact_utree);      \
    test_class(linear_utree);
#endif


// Simple testing framework
static size_t test_size = TEST_SIZE;

#if TEST_RUNTIME
using test_clock = std::chrono::high_resolution_clock;

static test_clock::time_point test_time_start;
static test_clock::time_point test_time_stop;
static test_clock::duration   test_time_duration;
#endif

#if TEST_INSTRUCTIONS
using test_cycle_t = uint64_t;

static test_cycle_t test_cycle_start;
static test_cycle_t test_cycle_stop;
static test_cycle_t test_cycle_duration;

static inline test_cycle_t test_cycle() {
    uint32_t a, b;
    __asm__ volatile ("rdtsc" : "=a" (a), "=d" (b));
    return ((uint64_t)b << 32) | (uint64_t)a; 
}
#endif

#if TEST_HEAP
static size_t test_heap_current;
static size_t test_heap_max;

extern "C" void *__libc_malloc(size_t size);
extern "C" void __libc_free(void *p);

extern "C" void *malloc(size_t size) throw () {
    test_heap_current += size;
    if (test_heap_current > test_heap_max) {
        test_heap_max = test_heap_current;
    }

    size_t *m = static_cast<size_t*>(__libc_malloc(size + sizeof(size)));
    m[0] = size;
    return &m[1];
}

extern "C" void free(void *p) throw () {
    if (!p) {
        return;
    }

    size_t *m = static_cast<size_t*>(p) - 1;
    test_heap_current -= m[0];
    __libc_free(&m[0]);
}
#endif

class test_random {
private:
    std::default_random_engine _rand;
    std::uniform_int_distribution<> _dis;

public:
    test_random(unsigned min, unsigned max)
        : _dis(min, max) {
    }

    unsigned operator()() {
        return _dis(_rand);
    }

    void seed(unsigned seed = decltype(_rand)::default_seed) {
        _dis.reset();
        _rand.seed(seed);
    }
};

static const std::unordered_map<double, std::string> test_prefixes = {
    {18,  "E"}, {15,  "P"}, {12,  "T"},
    {9,   "G"}, {6,   "M"}, {3,   "k"},
    {0,   ""}, 
    {-3,  "m"}, {-6,  "u"}, {-9,  "n"},
    {-12, "p"}, {-15, "f"}, {-18, "a"},
};

static std::string test_unitfy(double v, const std::string &u) {
    if (v == 0) {
        return "0" + u;
    }

    double unit = 3*std::floor(
        std::log(std::abs(v)) / std::log(1e3));

    std::ostringstream out;
    out << std::setprecision(3);
    out << v / std::pow(10, unit);
    out << test_prefixes.at(unit) << u;
    return out.str();
}

template <typename ...Ts>
static std::string test_unitfy(
        const std::chrono::duration<Ts...> &d, const std::string &u) {
    return test_unitfy(std::chrono::duration<double>(d).count(), u);
}


static inline void test_start() {
#if TEST_RUNTIME
    test_time_start = test_clock::now();
#endif
#if TEST_INSTRUCTIONS
    test_cycle_start = test_cycle();
#endif
#ifdef TEST_SETUP
    TEST_SETUP;
#endif
}

static inline void test_stop() {
#ifdef TEST_SETDOWN
    TEST_SETDOWN;
#endif
#if TEST_INSTRUCTIONS
    test_cycle_stop = test_cycle();
    test_cycle_duration += test_cycle_stop - test_cycle_start;
#endif
#if TEST_RUNTIME
    test_time_stop = test_clock::now();
    test_time_duration += test_time_stop - test_time_start;
#endif
}

template <template <typename ...> class M, typename F>
void test_case(const std::string &name, F test) {
#if TEST_RUNTIME
    test_clock::duration time_best = test_clock::duration::max();
#endif
#if TEST_INSTRUCTIONS
    test_cycle_t cycle_best = static_cast<test_cycle_t>(-1);
#endif
#if TEST_HEAP
    test_heap_current = 0;
    test_heap_max = 0;
#endif

    for (size_t runs = 0; runs < TEST_RUNS; runs++) {
#if TEST_RUNTIME
        test_time_duration = test_clock::duration::zero();
#endif
#if TEST_INSTRUCTIONS
        test_cycle_duration = 0;
#endif

        test();

#if TEST_RUNTIME
        if (test_time_duration < time_best) {
            time_best = test_time_duration;
        }
#endif
#if TEST_INSTRUCTIONS
        if (test_cycle_duration < cycle_best) {
            cycle_best = test_cycle_duration;
        }
#endif
    }

    std::cout << name << ": ";
#if TEST_RUNTIME
    std::cout << test_unitfy(time_best, "s") << " ";
#endif
#if TEST_INSTRUCTIONS
    std::cout << test_unitfy(cycle_best, "i") << " ";
#endif
#if TEST_HEAP
    std::cout << test_unitfy(test_heap_max, "B") << " "; 
#endif
    std::cout << std::endl;
}


// Test cases
template <template <typename ...> class M>
void lookups_test() {
    M<unsigned, unsigned> map;
    test_random rand(0, test_size);
    for (size_t i = 0; i < test_size; i++) {
        unsigned r = rand();
        map[r] = r;
    }

    test_start();
    for (size_t i = 0; i < test_size; i++) {
        unsigned r = rand();
        auto f = map.find(r);
        if (f != map.end()) {
            assert(f->second == r);
        }
    }
    test_stop();
}

template <template <typename ...> class M>
void insertions_test() {
    M<unsigned, unsigned> map;
    test_random rand(0, test_size);

    test_start();
    for (size_t i = 0; i < test_size; i++) {
        unsigned r = rand();
        map[r] = r;
    }
    test_stop();
}

template <template <typename ...> class M>
void deletions_test() {
    M<unsigned, unsigned> map;
    test_random rand(0, test_size);
    for (size_t i = 0; i < test_size; i++) {
        unsigned r = rand();
        map[r] = r;
    }

    test_start();
    for (size_t i = 0; i < test_size; i++) {
        unsigned r = rand();
        auto f = map.find(r);
        if (f != map.end()) {
            assert(f->second == r);
            map.erase(f);
        }
    }
    test_stop();
}

template <template <typename ...> class M>
void iteration_test() {
    M<unsigned, unsigned> map;
    test_random rand(0, test_size);
    for (size_t i = 0; i < test_size; i++) {
        unsigned r = rand();
        map[r] = r;
    }

    size_t count = 0;

    test_start();
    for (auto &p : map) {
        assert(p.first == p.second);
        count += 1;
    }
    test_stop();

    assert(map.size() == count);
}


// Entry point to testing
template <template <typename ...> class M>
void test_class(const std::string &name) {
    std::cout << "--- " << name << " ---" << std::endl;

#define test_case(F) test_case<M>(#F, F<M>)
    TEST_CASES;
#undef test_case

    std::cout << std::endl;
}

int main(int argc, char **argv) {
    if (argc >= 2) {
        test_size = std::stoi(argv[1]);
    }

#define test_class(M) test_class<M>(#M)
    TEST_CLASSES;
#undef test_class
}

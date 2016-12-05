#!/usr/bin/env python2

# Profiling framework for measuring the performance of tests
# using both internal and external measurements over exponentially
# growing data sets
#
# Copyright (c) 2016 Christopher Haster
# Distributed under the MIT license

import subprocess
import sys
import os
import re
import math
import itertools
import string
import json
import collections

import status
import shplot


# Configurable options pulled from the environment
TARGET   = os.getenv('TARGET', 'tests/tests')
CXX      = os.getenv('CXX', 'c++').split()
CXXFLAGS = os.getenv('CXXFLAGS', '-std=c++11 -I.').split()
VALGRIND = os.getenv('VALGRIND', 'valgrind').split()

TEST_CLASSES      = filter(None, os.getenv('TEST_CLASSES', '').split(','))
TEST_CASES        = filter(None, os.getenv('TEST_CASES', '').split(','))
TEST_MEASUREMENTS = filter(None, os.getenv('TEST_MEASUREMENTS', '').split(','))

TEST_SIZE       = int(os.getenv('TEST_SIZE', '8192'))
TEST_MIN        = int(os.getenv('TEST_MIN', '1024'))
TEST_MAX        = int(os.getenv('TEST_MAX', TEST_SIZE))
TEST_MULTIPLIER = float(os.getenv('TEST_MULTIPLIER', '2'))


# Available measurements
MEASUREMENTS = [
    ('runtime', 'i'),
    ('heap_usage', 'B'),
    ('L1_cache_misses', 'm'),
    ('L2_cache_misses', 'm'),
    ('branch_mispredicts', 'm'),
]

UNITS = collections.defaultdict(lambda: 'i', MEASUREMENTS)
PREFIXES = shplot.PREFIXES
COLORS = [c for c in shplot.COLORS if not re.search('(black|white)', c)]
LETTERS = string.lowercase

if not TEST_CLASSES:
    with open(TARGET+'.cpp') as file:
        for line in file:
            match = re.match('\s*test_class\(([\w:]*)\)', line)
            if match:
                TEST_CLASSES.append(match.group(1))

if not TEST_CASES:
    with open(TARGET+'.cpp') as file:
        for line in file:
            match = re.match('\s*test_case\(([\w:]*)\)', line)
            if match:
                TEST_CASES.append(match.group(1))

if not TEST_MEASUREMENTS:
    TEST_MEASUREMENTS = [m for m, _ in MEASUREMENTS]

TEST_MEASUREMENTS = list(reduce(itertools.chain,
    (TEST_CASES if m == 'runtime' else [m] for m in TEST_MEASUREMENTS), []))


def exponential(min=TEST_MIN, max=TEST_MAX, multiplier=TEST_MULTIPLIER):
    """ Lazily generates an exponential growth from min to max """
    i = min
    while i < max:
        yield i
        i *= multiplier
    yield i

def stylize():
    """ Generates pairs of color and letter for stylized output """
    return itertools.izip(itertools.cycle(COLORS), itertools.cycle(LETTERS))

def compile(classes=None, cases=None, flags=[]):
    """ Compiles the target program, caches the previous compile """ 
    command = CXX + CXXFLAGS

    if classes:
        command.append('-DTEST_CLASSES=' +
            ','.join('test_class(%s)' % c for c in 
                (classes if isinstance(classes, list) else [classes])))

    if cases:
        command.append('-DTEST_CASES=' +
            ','.join('test_case(%s)' % c for c in
                (cases if isinstance(cases, list) else [cases])))

    command.extend(flags)
    command.extend([TARGET+'.cpp', '-o', TARGET])

    if not hasattr(compile, 'last') or command != compile.last:
        subprocess.check_call(command, stderr=sys.stderr)
        compile.last = command

# Measurements integrated in the test program
def profile_runtime(class_, size):
    if not set(TEST_MEASUREMENTS) & ({'heap_usage'} | set(TEST_CASES)):
        return {}

    status.progress('compiling runtime measurements')
    compile(class_, TEST_CASES, flags=['-DTEST_RUNTIME=0'])

    status.progress('running runtime measurements (%d)' % size)
    command = [TARGET, '%d' % size]
    proc = subprocess.Popen(command, stdout=subprocess.PIPE)

    prefixes = {p: u for u, p in PREFIXES.items()}
    units = 'iB'
    result_pattern = re.compile(
        '([\d.]*)([%s]?)([%s])' % (''.join(prefixes), units))
    case_pattern = re.compile(
        '(\w*):([ \d.%s%s]*)' % (''.join(prefixes), units))

    cases = {}
    for line in proc.stdout:
        match = case_pattern.match(line)
        if match:
            name = match.group(1)
            case = result_pattern.findall(match.group(2))

            cases[name] = {unit: float(measurement) * 10**prefixes[prefix]
                           for measurement, prefix, unit in case}

    err = proc.wait()
    if err:
        raise subprocess.CalledProcessError(err, command)

    results = {name: case['i'] for name, case in cases.items()}

    mem = max(case['B'] for case in cases.values())
    results['heap_usage'] = mem

    return results

# Measurements from valgrind's callgrind tool
def profile_callgrind(class_, size):
    if not set(TEST_MEASUREMENTS) & {
            'L1_cache_misses',
            'L2_cache_misses',
            'branch_mispredicts'}:
        return {}

    status.progress('compiling callgrind measurements')
    compile(class_, TEST_CASES, flags=[
        '-DTEST_INSTRUCTIONS=0', '-DTEST_HEAP=0', '-DTEST_RUNS=1',
        '-include', 'valgrind/callgrind.h',
        '-DTEST_SETUP=CALLGRIND_TOGGLE_COLLECT',
        '-DTEST_SETDOWN=CALLGRIND_TOGGLE_COLLECT'])

    status.progress('running callgrind measurements (%d)' % size)
    output = 'tests/callgrind.out'
    command = VALGRIND + [
        '--tool=callgrind',
        '--callgrind-out-file=' + output,
        '--collect-atstart=no',
        '--cache-sim=yes', '--branch-sim=yes',
        TARGET, '%d' % size]

    with open(os.devnull, 'w') as DEVNULL:
        subprocess.check_call(command, stdout=DEVNULL, stderr=DEVNULL)

    status.progress('reading callgrind measurements')
    with open(output) as file:
        for line in file:
            if line.startswith('events: '):
                events = line.split()[1:]
                break

        for line in file:
            if line.startswith('summary: '):
                summary = map(int, line.split()[1:])
                break

    events = dict(zip(events,
        itertools.chain(summary, itertools.repeat(0))))
    results = [
        ('L1_cache_misses', events['I1mr']+events['D1mr']+events['D1mw']),
        ('L2_cache_misses', events['ILmr']+events['DLmr']+events['DLmw']),
        ('branch_mispredicts', events['Bcm']+events['Bim'])]

    return {m: r/len(TEST_CASES) for m, r in results}

# Finds test measurements, logs summary for each class
# as measurements are generated
def test_measurements():
    results = {c: {m: [] for m in TEST_MEASUREMENTS} for c in TEST_CLASSES}

    for class_ in TEST_CLASSES:
        with status.section(class_):
            runtimes = []
            callgrinds = []

            for size in exponential():
                runtime = profile_runtime(class_, size)
                for m in set(TEST_MEASUREMENTS) & set(runtime):
                    results[class_][m].append(runtime[m])

            for size in exponential():
                callgrind = profile_callgrind(class_, size)
                for m in set(TEST_MEASUREMENTS) & set(callgrind):
                    results[class_][m].append(callgrind[m])

            for m in TEST_MEASUREMENTS:
                status.result(m, shplot.unitfy(
                    results[class_][m][-1], UNITS[m]))

    return results

# Logs ordered results for each test measurement
def test_results(results):
    for m in TEST_MEASUREMENTS:
        with status.section(m):
            ordering = sorted(
                ((c, results[c][m][-1]) for c in TEST_CLASSES),
                key=lambda (_, r): r)

            for class_, result in ordering:
                status.result(class_, shplot.unitfy(result, UNITS[m]))

# Logs growth of each test measurement scaled per element
def test_growth(results):
    width1 = max(map(len, TEST_CLASSES))
    width2 = 18

    for m in TEST_MEASUREMENTS:
        with status.section(m):
            sh = shplot.ShPlot()
            scale = lambda rs: [r/size for r, size in zip(rs, exponential())]
            result = zip(((c, results[c][m], scale(results[c][m]))
                 for c in TEST_CLASSES), stylize())

            for (class_, _, ms), (color, letter) in result:
                sh.plot(ms, color=color, chars=2*letter+'.')

            sys.stdout.write('\n')
            sh.ylim(0)
            sh.dump()

            sys.stdout.write('\n')
            for (class_, rs, ms), (color, letter) in result:
                with shplot.color(color):
                    sys.stdout.write(5*' '+'%c %-*s' % (letter, width1, class_))
                sys.stdout.write('  %-*s' % (width2, '%s -> %s' % (
                    shplot.unitfy(ms[0], UNITS[m]+'/e'),
                    shplot.unitfy(ms[-1], UNITS[m]+'/e'))))
                sys.stdout.write('  %-*s' % (width2, '%s -> %s' % (
                    shplot.unitfy(rs[0], UNITS[m]),
                    shplot.unitfy(rs[-1], UNITS[m]))))
                sys.stdout.write('\n')

# Logs summary of all measurements per class
def test_summary(results):
    sh = shplot.ShPlot()
    avg = [sum(r[m][-1] for r in results.values()) / len(results)
           for m in TEST_MEASUREMENTS]
    summary = zip(((c, results[c]) for c in TEST_CLASSES), stylize())

    for (class_, ms), (color, letter) in summary:
        sh.plot([ms[m][-1]/a for m, a in zip(TEST_MEASUREMENTS, avg)],
            color=color, chars=letter+' .')

    sys.stdout.write('\n')
    sh.dump()

    sys.stdout.write('\n')
    for (class_, _), (color, letter) in summary:
        with shplot.color(color):
            sys.stdout.write(5*' '+'%c %s\n' % (letter, class_))

    sys.stdout.write('\n')
    for i, m in enumerate(TEST_MEASUREMENTS):
        sys.stdout.write(5*' '+'%d %s\n' % (i, m))


# Entry point for profiling
def main():
    with status.section('measurements'):
        results = test_measurements()

        results_file = os.path.join(os.path.dirname(TARGET), 'results.json')
        with open(results_file, 'w') as file:
            json.dump(results, file)

    with status.section('results'):
        test_results(results)

    with status.section('growth'):
        test_growth(results)

    with status.section('summary'):
        test_summary(results)

if __name__ == "__main__":
    main(*sys.argv[1:])

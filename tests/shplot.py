#!/usr/bin/env python2
#
# Simple plotter in the shell
#
# Copyright (c) 2016 Christopher Haster
# Distributed under the MIT license

"""
shplot - Simple plotter in the shell

This module provides a simple ascii-art plotter that can be used
in most shells without any graphical setup.

    import shplot

    sh = shplot.ShPlot()
    sh.plot([i   for i in range(10)], color="blue")
    sh.plot([i/2 for i in range(10)], color="green")
    sh.dump()

The shplot library can also be used directly from the shell.

    ./shplot.py data.json

"""

import sys
import math
import itertools
import json
import contextlib


# Default plot size as fallback
DEFAULT_WIDTH = 72
DEFAULT_HEIGHT = 20

# SI prefixes
PREFIXES = {
    18:  'E',
    15:  'P',
    12:  'T',
    9:   'G',
    6:   'M',
    3:   'k',
    0:   '',
    -3:  'm',
    -6:  'u',
    -9:  'n',
    -12: 'p',
    -15: 'f',
    -18: 'a',
}

def unitfy(n, u='', width=None):
    """ Unit formating, squishes and uses SI prefixes to save space """
    if n == 0:
        return '0' + u

    if width:
        prec = width - ((n < 0) + 2 + len(u))
    else:
        prec = 3

    n = float('%.*g' % (prec, n))
    unit = 3*math.floor(math.log(abs(n), 10**3))
    return '%.*g%s%s' % (prec, n/(10**unit), PREFIXES[unit], u)

# ANSI color codes
COLORS = {
    'black':            '\x1b[30m',
    'red':              '\x1b[31m',
    'green':            '\x1b[32m',
    'yellow':           '\x1b[33m',
    'blue':             '\x1b[34m',
    'magenta':          '\x1b[35m',
    'cyan':             '\x1b[36m',
    'white':            '\x1b[37m',
    'bright black':     '\x1b[30;1m',
    'bright red':       '\x1b[31;1m',
    'bright green':     '\x1b[32;1m',
    'bright yellow':    '\x1b[33;1m',
    'bright blue':      '\x1b[34;1m',
    'bright magenta':   '\x1b[35;1m',
    'bright cyan':      '\x1b[36;1m',
    'bright white':     '\x1b[37;1m',
}

COLOR_RESET = '\x1b[0m'

@contextlib.contextmanager
def color(color, file=sys.stdout):
    """ Coloring for file objects """
    if file.isatty() and color:
        file.write(COLORS[color])
        yield
        file.write(COLOR_RESET)
    else:
        yield

def line((x1, y1), (x2, y2)):
    """ Incremental error algorithm for rasterizing a line """
    dx = abs(x2-x1)
    dy = abs(y2-y1)
    sx = 1 if x1 < x2 else -1
    sy = 1 if y1 < y2 else -1
    err = dx - dy

    while True:
        yield x1, y1

        err2 = 2*err

        if x1 == x2 and y1 == y2:
            break

        if err2 > -dy:
            err -= dy
            x1 += sx

        if x1 == x2 and y1 == y2:
            break

        if err2 < dx:
            err += dx
            y1 += sy

    yield x2, y2

def ttydim(file):
    """ Try to get the terminal dimensions, may fail (ie on windows) """
    try:
        import fcntl, termios, struct
        height, width, _, _ = struct.unpack('HHHH',
            fcntl.ioctl(file.fileno(), termios.TIOCGWINSZ,
                struct.pack('HHHH', 0, 0, 0, 0)))
        return width, height
    except:
        return None

def isiter(i):
    """ Check if argument is iterable """
    return hasattr(i, '__iter__')


# Shell plotting class
class ShPlot:
    def __init__(self, width=None, height=None):
        """ Creates a shell plotter """
        self._dats = []
        self._width = width
        self._height = height
        self._xmin = None
        self._xmax = None
        self._ymin = None
        self._ymax = None

    def width(self, width):
        """
        Set width of plot, defaults to tty width when available
        otherwise arbitrarily 72
        """
        self._width = width

    def height(self, height):
        """
        Set height of plot, defaults to ratio of tty width when available
        otherwise arbitrarily 20
        """
        self._height = height

    def xlim(self, xmin=None, xmax=None):
        """
        Set the x-limits of the plot, defaults to min/max of platted data
        """
        if xmax is None and isiter(xmin):
            xmin, xmax = xmin

        self._xmin = xmin
        self._xmax = xmax

    def ylim(self, ymin=None, ymax=None):
        """
        Set the y-limits of the plot, defaults to min/max of plotted data
        """
        if ymax is None and isiter(ymin):
            ymin, ymax = ymin

        self._ymin = ymin
        self._ymax = ymax

    def plot(self, x=None, y=None, color=None, chars='oo.'):
        """
        Plot a set of data, most arguments are optional. Can take
        1-dimensional list, list of tuples or two lists of coordinates.

        Args:
            x: x values, defaults to 'range(0, len(y))'
            y: y values
            color: terminal color of data, available colors are in
                shplot.COLORS, defaults to no color
            chars: string of characters to draw data, with four uses:
                chars[0]: data point
                chars[1]: line interpolated between data points
                chars[2]: vertical line under the data point
                chars[3]: area under the data point
                defaults to 'oo.'
        """
        if not x and not y:
            return
        elif not y:
            y = x
            x = None

        y = list(y)
        if x:
            x = list(x)
        elif all(map(isiter, y)):
            x, y = map(list, zip(*y))
        else:
            x = range(len(y))

        self._dats.append({
            'x': map(float, x),
            'y': map(float, y),
            'color': color,
            'chars': chars
        })

    def _generate(self, width, height):
        """ Generate 2d map of points """
        assert len(self._dats) > 0
        m = {}

        xmin = self._xmin
        xmax = self._xmax
        ymin = self._ymin
        ymax = self._ymax

        if xmin is None: xmin = min(min(d['x']) for d in self._dats)
        if xmax is None: xmax = max(max(d['x']) for d in self._dats)
        if ymin is None: ymin = min(min(d['y']) for d in self._dats)
        if ymax is None: ymax = max(max(d['y']) for d in self._dats)

        if xmin == xmax or ymin == ymax:
            return m, (xmin, xmax), (ymin, ymax)

        xscale = lambda x: int((width-1) * ((x-xmin) / (xmax-xmin)))
        yscale = lambda y: int((height-1) * ((y-ymin) / (ymax-ymin)))

        flatten = lambda i: reduce(itertools.chain, i, [])
        repeat = itertools.repeat

        for dat in self._dats:
            z0 = zip(map(xscale, dat['x']), map(yscale, dat['y']))
            z1 = list(flatten(line(p0, p1) for p0, p1 in zip(z0, z0[1:])))
            z2 = flatten(zip(repeat(x), range(0, y)) for x, y in z0)
            z3 = flatten(zip(repeat(x), range(0, y)) for x, y in z1)

            for z, path in enumerate([z0, z1, z2, z3]):
                if len(dat['chars']) <= z or dat['chars'][z] == ' ':
                    continue

                for x, y in path:
                    if (x, y) in m and m[(x, y)][0] < z:
                        continue

                    m[(x, y)] = (z, dat)

        return m, (xmin, xmax), (ymin, ymax)

    def dump(self, file=sys.stdout):
        """ Dump the plot to a file object, defaults to stdout """
        width = self._width or DEFAULT_WIDTH
        height = self._height or DEFAULT_HEIGHT

        if file.isatty() and (not self._width or not self._height):
            dim = ttydim(file)
            if dim:
                width = self._width or min(dim[0]-8, DEFAULT_WIDTH)
                height = self._height or width*DEFAULT_HEIGHT/DEFAULT_WIDTH

        m, (xmin, xmax), (ymin, ymax) = self._generate(width, height)

        for y in reversed(range(height)):
            if y == height-1:
                file.write('%-5s^' % ('%4s' % unitfy(ymax, width=5)))
            else:
                file.write(5*' ' + '|')

            for x in range(width):
                if not (x, y) in m:
                    file.write(' ')
                    continue

                z, dat = m[(x, y)]

                with color(dat['color'], file):
                    file.write(dat['chars'][z:z+1])

            file.write('\n')

        file.write('%-5s+' % ('%4s' % unitfy(ymin, width=5)))
        file.write((width-1)*'-' + '>')
        file.write('\n')

        file.write(5*' ')
        file.write('%-5s' % unitfy(xmin, width=5))
        file.write((width-9)*' ')
        file.write('%5s' % unitfy(xmax, width=5))
        file.write('\n')

    def dumps(self):
        """ Dump the plot to a string """
        class strfile:
            def __init__(self):
                self._buffer = []

            def write(self, data):
                self._buffer.append(data)

            def isatty(self):
                return False

            def __str__(self):
                return ''.join(self._buffer)

        s = strfile()
        self.dump(s)
        return str(s)

# Entry point for standalone program
def main(*args):
    if not sys.stdin.isatty():
        input = sys.stdin
    elif len(args) >= 1:
        input = open(args[0], 'r')
    else:
        sys.stderr.write("Usage: %s <input.json>\n" % sys.argv[0])
        sys.exit(1)

    shplot = ShPlot()
    if len(args) >= 2:
        shplot.width(int(args[1]))
    if len(args) >= 3:
        shplot.height(int(args[2]))

    dats = json.load(input)
    if isinstance(dats, dict):
        dats = [dats]

    for dat in dats:
        for attr in ['width', 'height', 'xlim', 'ylim']:
            if attr in dat:
                getattr(shplot, attr)(dat[attr])
                del dat[attr]

        shplot.plot(**dat)

    shplot.dump(sys.stdout)

if __name__ == "__main__":
    main(*sys.argv[1:])

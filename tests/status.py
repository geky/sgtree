#!/usr/bin/env python2
#
# Simple status output for long-running shell programs
#
# Copyright (c) 2016 Christopher Haster
# Distributed under the MIT license

import sys
import contextlib


_section = None
_width = None

@contextlib.contextmanager
def section(section):
    """ Mark a new section """
    global _section

    _section = section
    sys.stdout.write('--- %s ---\n' % section)

    yield

    if _section == section:
        sys.stdout.write('\n')
        _section = None

def progress(case, state=None):
    """
    Shows progress, does not remain in output and is
    omitted if not a tty
    """
    global _width

    if not sys.stdout.isatty():
        return

    if _width:
        sys.stdout.write('\r' + _width*' ' + '\r')

    if case and state:
        status = '%s: %s... ' % (case, state)
    else:
        status = '%s... ' % case

    sys.stdout.write(status)
    sys.stdout.flush()
    _width = len(status)

def result(case, result):
    """ Shows a result, always output """
    global _width

    if sys.stdout.isatty():
        if _width:
            sys.stdout.write('\r' + _width*' ' + '\r')
        _width = None

    status = '%s: %s\n' % (case, result)
    sys.stdout.write(status)


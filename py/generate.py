#!/usr/bin/env python3

"""
Run this program as `py/generate.py src/core.h 1header/cut.h`
              or as `py/generate.py src/core.h 1header/cut.h --no-comment`
"""

import os
import re
import sys

INCLUDE = re.compile('^# *include "(.+\.h)"')

class Context(object):
    def __init__(self, path, comment):
        self._directory = os.path.dirname(path)
        self._name = os.path.basename(path)
        self._comment = comment
        self._top = self
        self._content = []
        self._processed = set()

    def inherit(self, path):
        new = Context(os.path.join(self._directory, path), self._comment)
        new._top = self._top
        return new

    def file(self):
        return os.path.join(self._directory, self._name)

    def addLine(self, l):
        if l[-1] != '\n':
            l += '\n'
        self._top._content.append(l)

    def addFile(self, name):
        ctx = self.inherit(name)
        self._top._processed.add(name)
        if self._comment:
            self.addLine('// 1hc substitution of {}\n'.format(ctx.file()))
        return ctx

    def addError(self):
        self.addLine('#error "file {} was not present in the current build"'.format(self.file()))

    def content(self):
        return ''.join(self._top._content)

    def alreadyProcessed(self, name):
        return True if name in self._top._processed else False

    def processFile(self, name):
        self._top._processed.add(name)


def examineLine(line, context):
    result = INCLUDE.match(line)
    if not result:
        return (False, '')
    name = result.group(1)
    return (context.alreadyProcessed(name), name)


def processFile(context):
    try:
        with open(context.file(), 'r') as f:
            for line in f:
                ignore, name = examineLine(line, context)
                if ignore:
                    continue
                if name:
                    processFile(context.addFile(name))
                else:
                    context.addLine(line)
    except (OSError, IOError) as e:
        context.addError()
        print('Failed to open {}'.format(context.file()))


def main():
    if len(sys.argv) < 3:
        print('invalid number of arguments')
        sys.exit(1)

    comment = False if len(sys.argv) > 3 and sys.argv[3] == '--no-comment' else True

    context = Context(sys.argv[1], comment)
    processFile(context)

    output_dir = os.path.dirname(sys.argv[2])
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)

    with open(sys.argv[2], 'w') as f:
        f.write(context.content())


if __name__ == '__main__':
    main()

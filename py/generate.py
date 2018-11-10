#!/usr/bin/env python3

"""
Run this program as `py/generate.py src/core.h 1header/cut.h`
              or as `py/generate.py src/core.h 1header/cut.h --no-comment`
"""

import os
import re
import sys

INCLUDE = re.compile('^# *include "(.+\.h)"')

class Global(object):
    def __init__(self):
        self._content = []

    def add(self, line):
        self._content.append(line)

    def content(self):
        return ''.join(self._content)


class Context(object):
    def __init__(self, path, comment):
        self._directory = os.path.dirname(path)
        self._name = os.path.basename(path)
        self._comment = comment
        self._global = Global()

    def inherit(self, path):
        new = Context(os.path.join(self._directory, path), self._comment)
        new._global = self._global
        return new

    def file(self):
        return os.path.join(self._directory, self._name)

    def addLine(self, l):
        if l[-1] != '\n':
            l += '\n'
        self._global.add(l)

    def addFile(self, name):
        ctx = self.inherit(name)
        if self._comment:
            self.addLine('// 1hc substitution of {}\n'.format(ctx.file()))
        return ctx

    def addError(self):
        self.addLine('#error "file {} was not present in the current build"'.format(self.file()))

    def content(self):
        return self._global.content()


def examineLine(line):
    result = INCLUDE.match(line)
    if not result:
        return None
    return result.group(1)


def processFile(context):
    try:
        with open(context.file(), 'r') as f:
            for line in f:
                include = examineLine(line)
                if include:
                    
                    processFile(context.addFile(include))
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

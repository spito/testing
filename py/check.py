#!/usr/bin/env python3

"""
Run this program as `py/check.py build/tests tests`
"""

import os
import sys
import platform
import subprocess

TEST_PREFIX = 't-'

class TestRunner(object):
    def __init__(self, test, outputDir):
        self.testDir, self.test = test
        self.testName = self.test[0:-4] if self.test[-4:] == '.exe' else self.test
        self.outputDir = outputDir
        self.statusUnknown = True

    def check(self, bootstrap):
        try:
            print('running {}: '.format(self.test), end='')
            p = subprocess.Popen([
                    os.path.join(self.testDir, self.test),
                    '--no-color',
                    '--short-path',
                    '{}'.format(len(self.testName))
                ],
                stdout=subprocess.PIPE)
            output, err = p.communicate()
            print(self._status(p.returncode))

            if bootstrap:
                return self._create(output)
            else:
                return self._compare(output)
        except FileNotFoundError as e:
            print('\tfile {} not found'.format(e.filename))
            return False

    def _status(self, returnCode):
        if self.testName[-4:] == 'pass':
            self.statusUnknown = False
            return 'OK' if returnCode == 0 else 'FAILED ({})'.format(returnCode)
        if self.testName[-4:] == 'fail':
            self.statusUnknown = False
            return 'OK' if returnCode != 0 else 'DID NOT FAILED'
        return 'UNKNOWN'

    def _outName(self):
        return os.path.join(self.outputDir, '{}.out'.format(self.testName))

    def _create(self, output):
        with open(self._outName(), 'wb') as f:
            f.write(output)
        return not self.statusUnknown

    def _compare(self, output):
        with open(self._outName(), 'rb') as f:
            return f.read(len(output) + 1) == output and not self.statusUnknown


def isexecutable(path):
    if not os.path.isfile(path):
        return False
    if platform.system() == 'Windows':
        return path[-4:] == '.exe'
    return os.access(path, os.X_OK)


def tests(path):
    return [
        (path, file) for file in os.listdir(path)
        if isexecutable(os.path.join(path, file))
    ]


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print('invalid number of arguments')
    bootstrap = True if len(sys.argv) > 3 and sys.argv[3] == '--bootstrap' else False

    problematic = []

    for test in tests(sys.argv[1]):
        runner = TestRunner(test, sys.argv[2])
        if not runner.check(bootstrap):
            problematic.append(test[1])

    if problematic:
        print('These tests are problematic:')
        for test in problematic:
            print('\t{}'.format(test))
    else:
        print('CUT seems to be OK')
    sys.exit(len(problematic))

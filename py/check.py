#!/usr/bin/env python3

"""
Run this program as `py/check.py build/tests tests`
              or as `py/check.py build/tests tests --bootstrap`
"""

import os
import sys
import platform
import subprocess

TEST_PREFIX = 't-'

class TestRunner(object):
    def __init__(self, test, outputDir):
        self._testDir, self._test = test
        self._testName = self._test[0:-4] if self._test[-4:] == '.exe' else self._test
        self._outputDir = outputDir
        self._statusUnknown = True

    def check(self, bootstrap):
        try:
            print('running {}: '.format(self._test), end='')
            p = subprocess.Popen([
                    os.path.join(self._testDir, self._test),
                    '--no-color',
                    '--short-path',
                    '{}'.format(len(self._testName))
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
        if self._testName[-4:] == 'pass':
            self._statusUnknown = False
            return 'OK' if returnCode == 0 else 'FAILED ({})'.format(returnCode)
        if self._testName[-4:] == 'fail':
            self._statusUnknown = False
            return 'OK' if returnCode != 0 else 'DID NOT FAILED'
        return 'UNKNOWN'

    def _outName(self):
        return os.path.join(self._outputDir, '{}.out'.format(self._testName))

    def _create(self, output):
        with open(self._outName(), 'wb') as f:
            f.write(output)
        return not self._statusUnknown

    def _compare(self, output):
        if self._statusUnknown:
            return False
        expected = dict()
        with open(self._outName(), 'r') as f:
            expected = {k: v for k, v in enumerate(f.read().splitlines())}
        given = {k: v for k, v in enumerate(output.decode('ascii').splitlines())}

        ok = len(expected) == len(given) and len(expected) == len([i for i in expected if i in given and expected[i] == given[i]])

        if not ok:
            print(f'Expected: {expected}\n')
            print(f'Given: {given}\n')

        return ok



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


def main():
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


if __name__ == '__main__':
    main()

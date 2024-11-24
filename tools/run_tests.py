#!/usr/bin/env python3
import os
import sys
import argparse
import subprocess as sp
import pathlib


def uint_gt2(arg: str) -> int:
    n = int(arg)
    if n < 2:
        raise ValueError
    return n


def args_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser()

    def_error = 3.7e-05
    p.add_argument('-e', '--error', type=float, default=def_error,
                   help='Acceptable error between actual and expected, ' \
                    f'default {def_error}')
    def_low = 2
    p.add_argument('-L', '--low', type=uint_gt2, default=def_low,
                   help=f'Lower bound for N to test, default {def_low}')
    def_high = 2
    p.add_argument('-H', '--high', type=uint_gt2, default=def_high,
                   help=f'High bound for N to test, default {def_high}')

    p.add_argument('sample', type=pathlib.Path)
    p.add_argument('benchmark', nargs='+', type=pathlib.Path)
    return p


def verbose_isfile(path: pathlib.Path) -> bool:
    if not os.path.isfile(path):
        print('# Unable to find', path)
        return False
    return True


def convert(line: str) -> float:
    return float(line.removeprefix('X = '))


def run(executable: pathlib.Path, N: int) -> list[float]:
    path = os.path.join('.', str(executable))
    pstate = sp.run([path, str(N)], stdout=sp.PIPE)
    if pstate.returncode != 0:
        return []
    lines = pstate.stdout.decode('utf-8').strip().split('\n')[:-1]
    return list(map(convert, lines))


def assert_diff(expected: list[float], actual: list[float], err: float) -> bool:
    for i, (e, a) in enumerate(zip(expected, actual)):
        diff = abs(e - a)
        if diff > err:
            print(f'# [{i}]: |{e} - {a}| = {diff} > {err}')
            return False
    return True


def main() -> int:
    args = args_parser().parse_args()

    print('TAP version 14')
    if not os.path.isfile(args.sample):
        print('# Unable to find', args.sample)
        print('Bail out!')
        return 1

    benchmarks = list(filter(verbose_isfile, args.benchmark))
    if not benchmarks:
        print('# No executables to verify')
        print('Bail out!')
        return 1

    tests = args.high + 1 - args.low
    print(f'1..{tests}')
    for n in range(args.low, args.high + 1):
        test_nr = n - args.low + 1
        expected_samples = run(args.sample, n)
        is_succeed = True
        for benchmark in benchmarks:
            actual_samples = run(benchmark, n)
            is_succeed = assert_diff(expected_samples, actual_samples,
                                     args.error) and is_succeed
            if not is_succeed:
                print('# Failed for', benchmark)
        if not is_succeed:
            print('not ', end='')
        print(f'ok {test_nr} - N =', n)
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print('Aborted!')
        sys.exit(1)


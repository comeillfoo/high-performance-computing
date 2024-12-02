#!/usr/bin/env python3
import sys
import os
import argparse
import pathlib


def args_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser()

    p.add_argument('table', type=pathlib.Path)

    p.add_argument('-n', '--parameters', type=int, default=1,
                   help='Number of columns before first result')

    def_directory=pathlib.Path('./data')
    p.add_argument('-C', '--directory', type=pathlib.Path, default=def_directory,
                   help='Path to directory where to save result CSV-tables, '
                   f'default {def_directory}')

    def_suffix='test'
    p.add_argument('-s', '--suffix', type=str, default=def_suffix,
                   help=f'Common suffix for CSV-tables, default {def_suffix}')
    return p


def main() -> int:
    args = args_parser().parse_args()

    rows = None
    with open(args.table) as fp:
        rows = fp.readlines()
    if rows is None:
        return 1

    thead = rows[0].strip().split(',')
    Ns = list(map(int, thead[args.parameters + 1:]))
    data = {}
    for i in range(1, len(rows)):
        tds = rows[i].strip().split(',')
        key = '-'.join(tds[0:args.parameters])
        test_nr = float(tds[args.parameters])
        times = list(map(float, tds[args.parameters + 1:]))
        subtest = data.get(key, {})
        for n, milliseconds in zip(Ns, times):
            subtest[n] = (subtest.get(n, 0.0) * test_nr + milliseconds) / (test_nr + 1.0)
        data[key] = subtest

    for subtest, results in data.items():
        filename = f'{subtest}-{args.suffix}.csv'
        path = os.path.join(args.directory, filename)
        with open(path, 'w') as fp:
            print('N,t', file=fp)
            for n, milliseconds in results.items():
                print(n, ',', milliseconds, sep='', file=fp)

    return 0

if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print('Aborted!')
        sys.exit(1)

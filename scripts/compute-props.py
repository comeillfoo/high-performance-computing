#!/usr/bin/env python3
import sys
import argparse
import pathlib

from typing import Tuple


def _read_scalability(lines: list[str]) -> dict:
    table = {}
    thN, thTime = list(map(str.strip, lines[0].split(',', 1)))
    tbody = list(map(lambda line: line.split(',', 1), lines[1:]))
    tdN = list(map(lambda trow: int(trow[0].strip()), tbody))
    tdTime = list(map(lambda trow: float(trow[1].strip()), tbody))

    table['thead'] = [thN, thTime]
    table['tbody'] = {
        thN: tdN,
        thTime: tdTime
    }
    return table


def read_scalability(path: pathlib.Path) -> dict:
    with open(path) as fp:
        return _read_scalability(fp.readlines())
    return {}


def csv2lines(table: dict) -> list[str]:
    lines = [ ','.join(table['thead']) + '\n' ]
    cols = [ table['tbody'][col] for col in table['thead'] ]
    lines.extend(map(lambda values: ','.join(map(str, values)) + '\n',
        zip(*cols)))
    return lines


def write_csv(path: pathlib.Path, table: dict) -> int:
    with open(path, 'w') as fp:
        fp.writelines(csv2lines(table))
    return 0


def S(args: Tuple[float, float]) -> float:
    v1, vp = args
    if v1 == 0.0:
        return vp / 1.0e-9
    return vp / v1


def compute_speedup(args: argparse.Namespace) -> int:
    unpar_table = read_scalability(args.unparallelized)
    par_table = read_scalability(args.parallelized)

    target_table = {
        'thead': ['N', 'S'],
        'tbody': {
            'N': unpar_table['tbody']['N'],
            'S': list(map(S, zip(unpar_table['tbody']['t'], par_table['tbody']['t'])))
        }
    }

    return write_csv(args.target, target_table)


def E_cb(p: int):
    assert p > 0
    def E(s: float) -> float:
        return s / p
    return E


def compute_efficiency(args: argparse.Namespace) -> int:
    source_table = read_scalability(args.source)

    target_table = {
        'thead': ['N', 'E'],
        'tbody': {
            'N': source_table['tbody']['N'],
            'E': list(map(E_cb(args.threads), source_table['tbody']['S']))
        }
    }
    return write_csv(args.target, target_table)


def args_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser()

    def_threads = 1
    p.add_argument('-p', '--threads', type=int, default=def_threads,
            help=f'Number of threads (cores, processes), default {def_threads}')

    subp = p.add_subparsers()

    comp_paraccel = subp.add_parser('speedup',
                                    help='Compute speedup')
    comp_paraccel.add_argument('unparallelized', type=pathlib.Path,
        help='Path to CSV-table with execution times for unparallelized version')
    comp_paraccel.add_argument('parallelized', type=pathlib.Path,
        help='Path to CSV-table with execution times for parallelized version')
    comp_paraccel.add_argument('target', type=pathlib.Path,
        help='Path to target CSV-table with computed speedups')
    comp_paraccel.set_defaults(func=compute_speedup)

    comp_pareffic = subp.add_parser('efficiency',
                                    help='Compute efficiency')
    comp_pareffic.add_argument('source', type=pathlib.Path,
        help='Path to source CSV-table with speedups')
    comp_pareffic.add_argument('target', type=pathlib.Path,
        help='Path to target CSV-table with computed efficiency')
    comp_pareffic.set_defaults(func=compute_efficiency)
    return p


def main() -> int:
    args = args_parser().parse_args()
    return args.func(args)


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print('Aborted!')
        sys.exit(1)
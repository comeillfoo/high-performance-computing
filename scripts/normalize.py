#!/usr/bin/env python3
import sys
import argparse
import pathlib


def args_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description='Normalizes table by first result')

    p.add_argument('input', type=pathlib.Path, help='Path to source table')
    return p


def main():
    args = args_parser().parse_args()
    lines = None
    with open(args.input) as f:
        lines = f.readlines()
    if not lines:
        print('Unable to read input table:', args.input)
        return 1

    skipped_header = False
    for line in lines:
        if not skipped_header:
            skipped_header = True
            print(line.strip())
            continue

        cols = line.split(',')
        Ys = list(map(float, cols[1:]))
        for i in range(1, len(Ys)):
            if Ys[0] == 0.0:
                continue
            Ys[i] /= Ys[0]
        if Ys[0] != 0.0:
            Ys[0] /= Ys[0]
        print(cols[0], *Ys, sep=',')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print('Aborted!')
        sys.exit(1)

#!/usr/bin/env python3

import argparse
import os
import sys
import logging
import pandas as pd
from matplotlib import pyplot as plt

LINE_WIDTH = 2


def plot_rtt(baseDir, outDir):
    df = pd.read_csv(os.path.join(baseDir, 'results.rtt.csv'))

    # Rename columns
    df = df.rename(columns={
        'rtt_with_invs': '9 invariants',
        'rtt_without_invs': '0 invariants',
    })
    columns = list(df.columns)

    # Plot RTT over time
    ax = df.plot(
        x=None,
        y=columns,
        kind='line',
        legend=False,
        xlabel='',
        ylabel='',
        rot=0,
        lw=LINE_WIDTH,
    )
    ax.grid(axis='both')
    ax.legend(bbox_to_anchor=(1, 1.12), ncol=len(columns), fontsize='large')
    ax.set_xlabel('Packets', fontsize='large')
    ax.set_ylabel('RTT (ms)', fontsize='large')
    ax.tick_params(axis='both', which='both', labelsize='large')
    ax.tick_params(axis='x', which='minor', left=False, right=False)
    fig = ax.get_figure()
    fn = os.path.join(outDir, ('rtt.png'))
    fig.savefig(fn, dpi=300)

    # Plot sorted RTT
    for col in columns:
        df[col] = df[col].sort_values(ascending=True).values
    ax = df.plot(
        x=None,
        y=columns,
        kind='line',
        legend=False,
        xlabel='',
        ylabel='',
        rot=0,
        lw=LINE_WIDTH,
    )
    ax.grid(axis='both')
    ax.legend(bbox_to_anchor=(1, 1.12), ncol=len(columns), fontsize='large')
    ax.set_xlabel('Packets', fontsize='large')
    ax.set_ylabel('RTT (ms)', fontsize='large')
    ax.tick_params(axis='both', which='both', labelsize='large')
    ax.tick_params(axis='x', which='minor', left=False, right=False)
    fig = ax.get_figure()
    fn = os.path.join(outDir, ('rtt.sorted.png'))
    fig.savefig(fn, dpi=300)


def main():
    parser = argparse.ArgumentParser(description='Plotting for Allie')
    parser.add_argument('-t',
                        '--target',
                        help='Plot target',
                        type=str,
                        choices=['all', 'rtt'],
                        action='store',
                        required=False,
                        default='all')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO,
                        format='[%(levelname)s] %(message)s')
    sourceDir = os.path.abspath(os.path.dirname(__file__))
    outDir = os.path.join(sourceDir, 'figures')
    os.makedirs(outDir, exist_ok=True)

    if args.target == 'all':
        plot_rtt(sourceDir, outDir)
    elif args.target == 'rtt':
        plot_rtt(sourceDir, outDir)
    else:
        logging.error("Unknown target: %s", args.target)


if __name__ == '__main__':
    main()

#!/usr/bin/env python3

import argparse
import os
import re
import logging
import pandas as pd


def parse_ping_log(filePath, outputList):
    if not os.path.exists(filePath):
        raise Exception('File not found: ' + filePath)

    with open(filePath) as logFile:
        for line in logFile:
            m = re.search(' time=([0-9.]+) ms$', line)
            if m != None:
                outputList.append(float(m.group(1)))


def parse_rtt(baseDir):
    results = {'rtt_with_invs': [], 'rtt_without_invs': []}

    logDir = os.path.join(baseDir, 'logs')
    pingT = os.path.join(logDir, 'ping-with-invs.txt')
    pingC = os.path.join(logDir, 'ping-without-invs.txt')
    parse_ping_log(pingT, results['rtt_with_invs'])
    parse_ping_log(pingC, results['rtt_without_invs'])
    df = pd.DataFrame.from_dict(results)
    df.to_csv(os.path.join(baseDir, 'results.rtt.csv'),
              encoding='utf-8',
              index=False)


def main():
    parser = argparse.ArgumentParser(description='Log parser for Allie')
    parser.add_argument('-t',
                        '--target',
                        help='Parser target',
                        type=str,
                        choices=['all', 'rtt'],
                        action='store',
                        required=False,
                        default='all')
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO,
                        format='[%(levelname)s] %(message)s')
    sourceDir = os.path.abspath(os.path.dirname(__file__))

    if args.target == 'all':
        parse_rtt(sourceDir)
    elif args.target == 'rtt':
        parse_rtt(sourceDir)
    else:
        logging.error("Unknown target: %s", args.target)


if __name__ == '__main__':
    main()

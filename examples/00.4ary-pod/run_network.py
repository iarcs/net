#!/usr/bin/env python3

import os, sys
import argparse

sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))
from src.network import Network


def get_host_ip(host_id):
    return '10.{}.{}.1/24'.format((host_id - 1) // 256, (host_id - 1) % 256)


def run_network(ecmp_enabled):
    cwd = os.path.abspath(os.path.dirname(__file__))
    network = Network(output_dir=os.path.join(cwd, 'output'))

    # Add hosts
    for h in range(1, 5):
        network.add_host('h{}'.format(h), get_host_ip(h))

    # Add switches
    for s in range(1, 5):
        network.add_switch('s{}'.format(s))

    # Tag groups
    network.add_group('s1', 'spine')
    network.add_group('s2', 'spine')
    network.add_group('s3', 'leaf')
    network.add_group('s4', 'leaf')

    # Add links
    network.add_link('s1', 's3', 1, 1)
    network.add_link('s1', 's4', 2, 1)
    network.add_link('s2', 's3', 1, 2)
    network.add_link('s2', 's4', 2, 2)
    network.add_link('s3', 'h1', 3)
    network.add_link('s3', 'h2', 4)
    network.add_link('s4', 'h3', 3)
    network.add_link('s4', 'h4', 4)

    # Add rules
    network.add_rule('s1', '10.0.0.0/23', 's3')
    network.add_rule('s1', '10.0.2.0/23', 's4')
    network.add_rule('s2', '10.0.0.0/23', 's3')
    network.add_rule('s2', '10.0.2.0/23', 's4')
    network.add_rule('s3', '10.0.0.0/24', 'h1')
    network.add_rule('s3', '10.0.1.0/24', 'h2')
    network.add_rule('s4', '10.0.2.0/24', 'h3')
    network.add_rule('s4', '10.0.3.0/24', 'h4')

    if not ecmp_enabled:
        # Static routes
        network.add_rule('s3', '10.0.2.0/23', 's1')
        network.add_rule('s4', '10.0.0.0/23', 's1')
    else:
        # With ECMP
        network.add_rule('s3', '10.0.2.0/23', 's1')
        network.add_rule('s3', '10.0.2.0/23', 's2')
        network.add_rule('s4', '10.0.0.0/23', 's1')
        network.add_rule('s4', '10.0.0.0/23', 's2')

    network.start()
    network.run_cli()
    network.stop()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='4ary-pod network')
    parser.add_argument('--ecmp',
                        action='store_true',
                        default=False,
                        help='Enable ECMP')
    args = parser.parse_args()

    run_network(args.ecmp)

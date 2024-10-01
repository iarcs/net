#!/usr/bin/env python3

import os, sys

sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))
from src.network import Network


def get_host_ip(host_id):
    return '10.{}.{}.1/24'.format((host_id - 1) // 256, (host_id - 1) % 256)


def run_network():
    cwd = os.path.abspath(os.path.dirname(__file__))
    network = Network(output_dir=os.path.join(cwd, 'output'))

    # Add hosts
    for h in range(1, 4):
        network.add_host('h{}'.format(h), get_host_ip(h))

    # Add switches
    for s in range(1, 4):
        network.add_switch('s{}'.format(s))

    # Add links
    network.add_link('s1', 's2', 1, 2)
    network.add_link('s2', 's3', 1, 2)
    network.add_link('s3', 's1', 1, 2)
    network.add_link('s1', 'h1', 3)
    network.add_link('s2', 'h2', 3)
    network.add_link('s3', 'h3', 3)

    # Add rules
    network.add_rule('s1', '10.0.0.0/24', 'h1')
    network.add_rule('s2', '10.0.1.0/24', 'h2')
    network.add_rule('s3', '10.0.2.0/24', 'h3')
    # Correct configuration
    network.add_rule('s1', '10.0.1.0/24', 's2')
    network.add_rule('s1', '10.0.2.0/24', 's3')
    network.add_rule('s2', '10.0.0.0/24', 's1')
    network.add_rule('s2', '10.0.2.0/24', 's3')
    network.add_rule('s3', '10.0.0.0/24', 's1')
    network.add_rule('s3', '10.0.1.0/24', 's2')
    # Incorrect configuration
    # network.add_rule('s1', '0.0.0.0/0', 's2')
    # network.add_rule('s2', '0.0.0.0/0', 's3')
    # network.add_rule('s3', '0.0.0.0/0', 's1')

    # With ECMP
    # network.add_rule('s1', '0.0.0.0/0', 's2')
    # network.add_rule('s1', '0.0.0.0/0', 's3')
    # network.add_rule('s2', '0.0.0.0/0', 's1')
    # network.add_rule('s2', '0.0.0.0/0', 's3')
    # network.add_rule('s3', '0.0.0.0/0', 's1')
    # network.add_rule('s3', '0.0.0.0/0', 's2')

    network.start()
    network.run_cli()
    network.stop()


if __name__ == '__main__':
    run_network()

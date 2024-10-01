#!/usr/bin/env python3

import os, sys
import argparse

sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))
from src.network import Network


def get_host_ip(k, host_id):
    pod_id = (host_id - 1) // (k // 2)**2
    host_id_in_pod = (host_id - 1) % (k // 2)**2
    return '10.{}.{}.1/24'.format(pod_id, host_id_in_pod)


def get_host_subnet(k, host_id):
    pod_id = (host_id - 1) // (k // 2)**2
    host_id_in_pod = (host_id - 1) % (k // 2)**2
    return '10.{}.{}.0/24'.format(pod_id, host_id_in_pod)


def run_network(k, ecmp_enabled):
    if k % 2 != 0 or k < 4 or k > 30:
        raise ValueError('Invalid k: {}'.format(k))

    cwd = os.path.abspath(os.path.dirname(__file__))
    network = Network(output_dir=os.path.join(cwd, 'output'))

    # Add hosts
    for h in range(1, 1 + k**3 // 4):
        network.add_host('h{}'.format(h), get_host_ip(k, h))

    # Add switches
    for s in range(1, 1 + (k // 2)**2 + k**2):
        network.add_switch('s{}'.format(s))
    index = 1
    core_switches = range(index, index + (k // 2)**2)
    index += len(core_switches)
    aggr_switches = range(index, index + k**2 // 2)
    index += len(aggr_switches)
    edge_switches = range(index, index + k**2 // 2)
    index += len(edge_switches)
    del index

    # Add links
    ## Core-Aggregation links
    for i, core in enumerate(core_switches):
        for j in range(0, k):
            aggr = aggr_switches[j * (k // 2) + i // (k // 2)]
            network.add_link('s{}'.format(core), 's{}'.format(aggr), j + 1,
                             i % (k // 2) + 1)
    ## Aggregation-Edge links
    for i, aggr in enumerate(aggr_switches):
        for j in range(0, k // 2):
            edge = edge_switches[(i - i % (k // 2)) + j]
            network.add_link('s{}'.format(aggr), 's{}'.format(edge),
                             j + k // 2 + 1, i % (k // 2) + 1)
    ## Edge-Host links
    for i, edge in enumerate(edge_switches):
        for j in range(0, k // 2):
            host = i * (k // 2) + j + 1
            network.add_link('s{}'.format(edge), 'h{}'.format(host),
                             j + k // 2 + 1)

    # Add downlink rules
    ## Core-Aggregation links
    for i, core in enumerate(core_switches):
        for j in range(0, k):
            aggr = aggr_switches[j * (k // 2) + i // (k // 2)]
            network.add_rule('s{}'.format(core), '10.{}.0.0/16'.format(j),
                             's{}'.format(aggr))
    ## Aggregation-Edge links
    for i, aggr in enumerate(aggr_switches):
        for j in range(0, k // 2):
            edge = edge_switches[(i - i % (k // 2)) + j]
            for m in range(0, k // 2):
                host = ((i - i % (k // 2)) + j) * (k // 2) + m + 1
                network.add_rule('s{}'.format(aggr), get_host_subnet(k, host),
                                 's{}'.format(edge))
    ## Edge-Host links
    for i, edge in enumerate(edge_switches):
        for j in range(0, k // 2):
            host = i * (k // 2) + j + 1
            network.add_rule('s{}'.format(edge), get_host_subnet(k, host),
                             'h{}'.format(host))

    # Add uplink rules
    if ecmp_enabled:
        ## Aggregation-Core links
        for i, aggr in enumerate(aggr_switches):
            for j in range(0, k // 2):
                core = core_switches[(i % (k // 2)) * (k // 2) + j]
                network.add_rule('s{}'.format(aggr), '0.0.0.0/0',
                                 's{}'.format(core))
        ## Edge-Aggregation links
        for i, edge in enumerate(edge_switches):
            for j in range(0, k // 2):
                aggr = aggr_switches[(i - i % (k // 2)) + j]
                network.add_rule('s{}'.format(edge), '0.0.0.0/0',
                                 's{}'.format(aggr))
    else:
        ## Aggregation-Core links
        for i, aggr in enumerate(aggr_switches):
            core = core_switches[(i % (k // 2)) * (k // 2)]
            network.add_rule('s{}'.format(aggr), '0.0.0.0/0',
                             's{}'.format(core))
        ## Edge-Aggregation links
        for i, edge in enumerate(edge_switches):
            aggr = aggr_switches[i - i % (k // 2)]
            network.add_rule('s{}'.format(edge), '0.0.0.0/0',
                             's{}'.format(aggr))

    network.start()
    network.run_cli()
    network.stop()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Fat-tree network')
    parser.add_argument('-k',
                        '--arity',
                        action='store',
                        default=4,
                        help='Fat-tree arity')
    parser.add_argument('--ecmp',
                        action='store_true',
                        default=False,
                        help='Enable ECMP')
    args = parser.parse_args()

    run_network(int(args.arity), args.ecmp)

#!/usr/bin/env python3

import json
from collections import defaultdict


def ecmp_select_entry(ip_prefix, group_id, ecmp_count):
    ip_addr = ip_prefix.split('/', 1)[0]
    prefix_len = int(ip_prefix.split('/', 1)[1])
    table_entry = {
        'table_name': 'MyIngress.ipv4_forwarding.ipv4_lpm',
        'match_fields': {
            'hdr.ipv4.dstAddr': (ip_addr, prefix_len)
        },
        'action_name': 'MyIngress.ipv4_forwarding.ecmp_select',
        'action_params': {
            'ecmp_group_id': group_id,
            'ecmp_count': ecmp_count
        }
    }
    if prefix_len == 0:
        del table_entry['match_fields']['hdr.ipv4.dstAddr']
    return table_entry


def ecmp_forward_entry(group_id, select_id, dst_eth_addr, egress_port):
    table_entry = {
        'table_name': 'MyIngress.ipv4_forwarding.ecmp_forward',
        'match_fields': {
            'meta.ecmp_group_id': group_id,
            'meta.ecmp_select': select_id
        },
        'action_name': 'MyIngress.ipv4_forwarding.ipv4_forward',
        'action_params': {
            'dstAddr': dst_eth_addr,
            'port': egress_port
        }
    }
    return table_entry


class ECMP:

    def __init__(self, network):
        self.network = network
        self.paths = defaultdict(lambda: defaultdict(set))
        # src_node -> (ip_prefix -> set{dst_nodes})

    def add_path(self, from_node, ip_prefix, to_node):
        self.paths[from_node][ip_prefix].add(to_node)

    def get_rules(self):
        rules = defaultdict(set) # switch name -> set{ json_str(table entries) }
        for node_name, local_paths in self.paths.items():
            group_id = 1
            for ip_prefix, next_hops in local_paths.items():
                rule = ecmp_select_entry(ip_prefix, group_id, len(next_hops))
                rules[node_name].add(json.dumps(rule, sort_keys=True))
                select_id = 0
                for next_hop in next_hops:
                    src_node = self.network.get(node_name)
                    dst_node = self.network.get(next_hop)
                    connections = src_node.connectionsTo(dst_node)
                    for src_intf, dst_intf in connections:
                        dst_eth_addr = dst_intf.MAC()
                        egress_port = src_node.ports[src_intf]
                        rule = ecmp_forward_entry(group_id, select_id,
                                                  dst_eth_addr, egress_port)
                        rules[node_name].add(json.dumps(rule, sort_keys=True))
                    select_id += 1
                group_id += 1
        return rules

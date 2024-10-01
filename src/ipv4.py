#!/usr/bin/env python3

import json
from collections import defaultdict


def ipv4_lpm_entry(ip_prefix, dst_eth_addr, egress_port):
    ip_addr = ip_prefix.split('/', 1)[0]
    prefix_len = int(ip_prefix.split('/', 1)[1])
    table_entry = {
        'table_name': 'MyIngress.ipv4_forwarding.ipv4_lpm',
        'match_fields': {
            'hdr.ipv4.dstAddr': (ip_addr, prefix_len)
        },
        'action_name': 'MyIngress.ipv4_forwarding.ipv4_forward',
        'action_params': {
            'dstAddr': dst_eth_addr,
            'port': egress_port
        }
    }
    if prefix_len == 0:
        del table_entry['match_fields']['hdr.ipv4.dstAddr']
    return table_entry


class IPv4:

    def __init__(self, network):
        self.network = network
        self.rules = defaultdict(set)
        # switch name -> set{ json_str(table entries) }

    def add_fwd_rule(self, from_node, ip_prefix, to_node):
        src_node = self.network.get(from_node)
        dst_node = self.network.get(to_node)
        assert (isinstance(src_node, self.network.net.switch))
        connections = src_node.connectionsTo(dst_node)
        for src_intf, dst_intf in connections:
            dst_eth_addr = dst_intf.MAC()
            egress_port = src_node.ports[src_intf]
            rule = ipv4_lpm_entry(ip_prefix, dst_eth_addr, egress_port)
            self.rules[from_node].add(json.dumps(rule, sort_keys=True))

    def get_rules(self):
        return self.rules

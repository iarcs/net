#!/usr/bin/env python3

import re
import json
from abc import *
from collections import defaultdict

from .constants import *
from .dfa import DFA
from .packetset import PacketSet
from .regex import Regex


class Invariant:

    def __init__(self, inv_id, inv_spec):
        self.id = inv_id
        self.name = inv_spec['name']
        self.type = inv_spec['type']
        self.packet_set = PacketSet(inv_spec['packet_set'])

    def __str__(self):
        return ('Invariant ' + str(self.id) + '\n' + 'Name: ' + self.name +
                '\n' + 'Type: ' + self.type + '\n' + 'Packet Set: ' +
                str(self.packet_set))

    @abstractmethod
    def get_rules(self, network):
        raise Exception()


class RegexInvariant(Invariant):

    def __init__(self, inv_id, inv_spec):
        super().__init__(inv_id, inv_spec)
        self.pattern = inv_spec['pattern']
        self.regex = None
        self.dfa = None

    def __str__(self):
        return (super().__str__() + '\n' + 'Pattern: ' + self.pattern)

    def get_rules(self, network):
        if self.regex == None:
            self.regex = Regex(self.pattern, network)
        if self.dfa == None:
            self.dfa = DFA(self.regex, network, fn=self.name)

        rules = defaultdict(list)
        ep_pattern = re.compile(r'(s\d+)(p(\d+)|d)', flags=re.IGNORECASE)

        # DFA initialization rules
        initialTransitions = self.dfa.transitions[self.dfa.initial]
        for input_, next_state in initialTransitions.items():
            m = ep_pattern.match(input_)
            sw_name = m.group(1)
            assert m.group(2).lower() != 'd'
            in_port = int(m.group(3))
            # Only check at the border switches
            if len(network['switches'][sw_name]['host_ports']) == 0:
                continue
            rules[sw_name].append({
                'table_name': 'MyIngress.regex_init',
                'match_fields': {
                    'hdr.ipv4.srcAddr': self.packet_set.src_ip(),
                    'hdr.ipv4.dstAddr': self.packet_set.dst_ip(),
                    'meta.verification.entering': 1,
                    'std_meta.ingress_port': (in_port, in_port)
                },
                'action_name': 'MyIngress.initial_transition',
                'action_params': {
                    'state': next_state
                },
                'priority': Priority.HIGH
            })
        for sw_name, sw_dict in network['switches'].items():
            # Only check at the border switches
            if len(sw_dict['host_ports']) == 0:
                continue
            rules[sw_name].append({
                'table_name': 'MyIngress.regex_init',
                'match_fields': {
                    'hdr.ipv4.srcAddr': self.packet_set.src_ip(),
                    'hdr.ipv4.dstAddr': self.packet_set.dst_ip(),
                    'meta.verification.entering': 1,
                },
                'action_name': 'MyIngress.violate',
                'action_params': {
                    'invId': self.id
                },
                'priority': Priority.LOW
            })

        # DFA transition rules
        for curr_state, trans in self.dfa.transitions.items():
            for input_, next_state in trans.items():
                m = ep_pattern.match(input_)
                sw_name = m.group(1)
                if m.group(2).lower() == 'd':
                    rules[sw_name].append({
                        'table_name': 'MyIngress.regex_transition',
                        'match_fields': {
                            'hdr.ipv4.srcAddr': self.packet_set.src_ip(),
                            'hdr.ipv4.dstAddr': self.packet_set.dst_ip(),
                            'hdr.verification.dfaState':
                                (curr_state, curr_state),
                            'std_meta.egress_spec': DROP_PORT
                        },
                        'action_name': 'MyIngress.regex_trans',
                        'action_params': {
                            'state': next_state
                        },
                        'priority': Priority.HIGH
                    })
                    rules[sw_name].append({
                        'table_name': 'MyEgress.regex_transition',
                        'match_fields': {
                            'hdr.ipv4.srcAddr': self.packet_set.src_ip(),
                            'hdr.ipv4.dstAddr': self.packet_set.dst_ip(),
                            'hdr.verification.dfaState':
                                (curr_state, curr_state),
                            'std_meta.egress_spec': (DROP_PORT, DROP_PORT)
                        },
                        'action_name': 'MyEgress.regex_trans',
                        'action_params': {
                            'state': next_state
                        },
                        'priority': Priority.HIGH
                    })
                else:
                    out_port = int(m.group(3))
                    rules[sw_name].append({
                        'table_name': 'MyEgress.regex_transition',
                        'match_fields': {
                            'hdr.ipv4.srcAddr': self.packet_set.src_ip(),
                            'hdr.ipv4.dstAddr': self.packet_set.dst_ip(),
                            'hdr.verification.dfaState':
                                (curr_state, curr_state),
                            'std_meta.egress_port': (out_port, out_port)
                        },
                        'action_name': 'MyEgress.regex_trans',
                        'action_params': {
                            'state': next_state
                        },
                        'priority': Priority.MEDIUM
                    })
        for sw_name in network['switches'].keys():
            rules[sw_name].append({
                'table_name': 'MyEgress.regex_transition',
                'match_fields': {
                    'hdr.ipv4.srcAddr': self.packet_set.src_ip(),
                    'hdr.ipv4.dstAddr': self.packet_set.dst_ip(),
                },
                'action_name': 'MyEgress.violate',
                'action_params': {
                    'invId': self.id
                },
                'priority': Priority.LOW
            })

        # DFA termination rules
        for sw_name, sw_dict in network['switches'].items():
            # Only check at the border switches
            if len(sw_dict['host_ports']) == 0:
                continue

            rules[sw_name].append({
                'table_name': 'MyEgress.regex_terminate',
                'match_fields': {
                    'hdr.ipv4.srcAddr': self.packet_set.src_ip(),
                    'hdr.ipv4.dstAddr': self.packet_set.dst_ip(),
                    'meta.verification.leaving': 1,
                },
                'action_name': 'MyEgress.violate',
                'action_params': {
                    'invId': self.id
                },
                'priority': Priority.LOW
            })
            for state in self.dfa.accepting:
                rules[sw_name].append({
                    'table_name': 'MyEgress.regex_terminate',
                    'match_fields': {
                        'hdr.ipv4.srcAddr': self.packet_set.src_ip(),
                        'hdr.ipv4.dstAddr': self.packet_set.dst_ip(),
                        'meta.verification.leaving': 1,
                        'hdr.verification.dfaState': (state, state),
                    },
                    'action_name': 'NoAction',
                    'action_params': {},
                    'priority': Priority.HIGH
                })

        return rules


class SegmentationInvariant(Invariant):

    def __init__(self, inv_id, inv_spec):
        super().__init__(inv_id, inv_spec)
        self.switch = inv_spec['switch']
        self.port = inv_spec['port']

    def __str__(self):
        return (super().__str__() + '\n' + 'Switch: ' + self.switch + '\n' +
                'Port: ' + str(self.port))

    def get_rules(self, network):
        rules = defaultdict(list)

        # If a packet in the packet set reaches the <switch, port>, a violation
        # occurs.
        rules[self.switch].append({
            'table_name': 'MyEgress.segmentation',
            'match_fields': {
                'hdr.ipv4.srcAddr': self.packet_set.src_ip(),
                'hdr.ipv4.dstAddr': self.packet_set.dst_ip(),
                'std_meta.egress_spec': (DROP_PORT, DROP_PORT)
            },
            'action_name': 'NoAction',
            'priority': Priority.HIGH
        })
        rules[self.switch].append({
            'table_name': 'MyEgress.segmentation',
            'match_fields': {
                'hdr.ipv4.srcAddr': self.packet_set.src_ip(),
                'hdr.ipv4.dstAddr': self.packet_set.dst_ip(),
                'std_meta.egress_port': (self.port, self.port)
            },
            'action_name': 'MyEgress.violate',
            'action_params': {
                'invId': self.id
            },
            'priority': Priority.LOW
        })

        return rules


class LoopInvariant(Invariant):

    def __init__(self, inv_id, inv_spec):
        super().__init__(inv_id, inv_spec)

    def __str__(self):
        return super().__str__()

    def get_rules(self, network):
        return dict() # Disable loop checks for now
        rules = defaultdict(list)

        for sw_name, sw_dict in network['switches'].items():
            sw_id = sw_dict['device_id']
            for trace_idx in range(TRACE_LENGTH):
                rules[sw_name].append({
                    'table_name': 'MyIngress.loop',
                    'match_fields': {
                        'hdr.verification.traceCount':
                            (trace_idx + 1, TRACE_LENGTH),
                        'hdr.traces[' + str(trace_idx) + '].swId':
                            (sw_id, sw_id),
                    },
                    'action_name': 'MyIngress.violate',
                    'action_params': {
                        'invId': self.id
                    },
                    'priority': Priority.LOW
                })

        return rules


invariant_classes = {
    'regex': RegexInvariant,
    'segmentation': SegmentationInvariant,
    'loop': LoopInvariant,
}


class InvariantsParser:

    @staticmethod
    def parse(invariants_json):
        invariants = []

        with open(invariants_json, 'r') as infile:
            invs = json.load(infile)

            for inv_id, inv_spec in enumerate(invs):
                inv_cls = invariant_classes[inv_spec['type']]
                invariant = inv_cls(inv_id, inv_spec)
                invariants.append(invariant)

        return invariants

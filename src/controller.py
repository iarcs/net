#!/usr/bin/env python3

import grpc
import json
import time
import asyncio
from collections import defaultdict
from pypacker.layer12 import ethernet
from pypacker.layer3 import ip, icmp

from .p4runtime_lib.bmv2 import Bmv2SwitchConnection
from .p4runtime_lib.helper import P4InfoHelper
from .p4runtime_lib.error_utils import printGrpcError
from .p4runtime_lib.switch import ShutdownAllSwitchConnections
from .constants import *
from .invariants import *
from .network import run_mnexc_cmd


class Controller:

    def __init__(self, network_json, inv_json, p4info_path, bmv2_json):
        # Load the network
        self.network = {}
        with open(network_json, 'r') as infile:
            self.network = json.load(infile)

        # Load the invariants
        self.invariants = InvariantsParser.parse(inv_json)

        # TODO: Calculate the invariant-wide equivalent packet sets
        self.ps_to_invs = dict() # PacketSet -> [ Invariants ]
        self.ps_to_dfas = dict() # PacketSet -> DFA

        # Switch connections
        self.sw_conns = {}
        self.p4info_helper = P4InfoHelper(p4info_path)
        self.bmv2_json = bmv2_json
        self.futures = []

    def set_reduced_MTU(self):
        if len(self.invariants) > 0:
            for host_dict in self.network['hosts'].values():
                for intf in host_dict['intfs'].keys():
                    # MTU: 1500 - verification header length
                    cmd = 'ip link set dev {} mtu 1497'.format(intf)
                    run_mnexc_cmd(host_dict['pid'], cmd)

    def connect(self):
        for sw in self.network['switches'].values():
            p4rt_logs = 'logs/{}-p4runtime-requests.txt'.format(sw['name'])
            sw_conn = Bmv2SwitchConnection(name=sw['name'],
                                           address='127.0.0.1:{}'.format(
                                               sw['grpc_port']),
                                           device_id=sw['device_id'],
                                           proto_dump_file=p4rt_logs)
            self.sw_conns[sw['name']] = sw_conn

            # Send master arbitration update message to establish this
            # controller as master (required by P4Runtime before performing any
            # other write operation)
            sw_conn.MasterArbitrationUpdate()

    def install_P4_programs(self):
        for sw_conn in self.sw_conns.values():
            print('Installing P4 program on', sw_conn.name)
            sw_conn.SetForwardingPipelineConfig(
                p4info=self.p4info_helper.p4info,
                bmv2_json_file_path=self.bmv2_json)

    def install_rules(self, rules_dict):
        num_rules = 0
        for sw_name, rules in rules_dict.items():
            for rule in rules:
                tbl_name = rule['table_name']
                mfs = rule['match_fields'] if 'match_fields' in rule else None
                an = rule['action_name'] if 'action_name' in rule else None
                aps = rule['action_params'] if 'action_params' in rule else None
                priority = rule['priority'] if 'priority' in rule else None
                table_entry = self.p4info_helper.buildTableEntry(
                    table_name=tbl_name,
                    match_fields=mfs,
                    action_name=an,
                    action_params=aps,
                    priority=priority)
                # print(sw_name, json.dumps(rule, indent=4))
                self.sw_conns[sw_name].WriteTableEntry(table_entry)
            num_rules += len(rules)
        return num_rules

    def _install_encap_decap_rules(self):
        num_rules = 0
        for sw_name, sw in self.network['switches'].items():
            # Only install at the border switches
            if len(sw['host_ports']) == 0:
                continue

            # Install rules for entering the network
            table_entry = self.p4info_helper.buildTableEntry(
                table_name='MyIngress.encapsulation',
                match_fields={
                    'hdr.ipv4.protocol':
                        (PROTO_VERIFICATION, PROTO_VERIFICATION)
                },
                action_name='NoAction',
                priority=Priority.HIGH)
            self.sw_conns[sw_name].WriteTableEntry(table_entry)

            table_entry = self.p4info_helper.buildTableEntry(
                table_name='MyIngress.encapsulation',
                match_fields={},
                action_name='MyIngress.insert_verification_header',
                priority=Priority.LOW)
            self.sw_conns[sw_name].WriteTableEntry(table_entry)

            # Install rules for leaving the network
            for host_port in sw['host_ports']:
                table_entry = self.p4info_helper.buildTableEntry(
                    table_name='MyEgress.check_leaving',
                    match_fields={
                        'std_meta.egress_port': host_port,
                    },
                    action_name='MyEgress.mark_leaving',
                )
                self.sw_conns[sw_name].WriteTableEntry(table_entry)

            # Install rules for removing verification headers
            table_entry = self.p4info_helper.buildTableEntry(
                table_name='MyEgress.decapsulation',
                match_fields={'meta.verification.leaving': 1},
                action_name='MyEgress.remove_verification_header')
            self.sw_conns[sw_name].WriteTableEntry(table_entry)

            num_rules += 3 + len(sw['host_ports'])
        return num_rules

    # def _install_trace_rules(self):
    #     for sw_name, sw in self.network['switches'].items():
    #         table_entry = self.p4info_helper.buildTableEntry(
    #             table_name='MyIngress.trace',
    #             match_fields={
    #                 'hdr.verification.traceCount': (0, TRACE_LENGTH - 1),
    #                 'meta.verification.leaving': 0
    #             },
    #             action_name='MyIngress.add_trace',
    #             action_params={
    #                 'add': 1,
    #                 'swId': sw['device_id']
    #             },
    #             priority=Priority.LOW)
    #         self.sw_conns[sw_name].WriteTableEntry(table_entry)

    #         table_entry = self.p4info_helper.buildTableEntry(
    #             table_name='MyIngress.trace',
    #             match_fields={
    #                 'hdr.verification.traceCount': (TRACE_LENGTH, TRACE_LENGTH),
    #                 'meta.verification.leaving': 0
    #             },
    #             action_name='MyIngress.add_trace',
    #             action_params={
    #                 'add': 0,
    #                 'swId': sw['device_id']
    #             },
    #             priority=Priority.LOW)
    #         self.sw_conns[sw_name].WriteTableEntry(table_entry)

    def install_verification_rules(self):
        num_rules = 0

        if len(self.invariants) > 0:
            num_rules += self._install_encap_decap_rules()
            # num_rules += self._install_trace_rules() # Disable for now

        installed_inv_rules = defaultdict(set)

        def _is_installed(sw_name, rule):
            nonlocal installed_inv_rules
            match_entry = {k: rule[k] for k in ['table_name', 'match_fields']}
            return (json.dumps(match_entry, sort_keys=True)
                    in installed_inv_rules[sw_name])

        def _add_installed_rule(sw_name, rule):
            nonlocal installed_inv_rules
            match_entry = {k: rule[k] for k in ['table_name', 'match_fields']}
            installed_inv_rules[sw_name].add(
                json.dumps(match_entry, sort_keys=True))

        compile_time = 0
        install_time = 0

        for invariant in self.invariants:
            print('Installing rules for invariant', invariant.name)
            # Compile the rules
            start = time.perf_counter()
            rules = invariant.get_rules(self.network)
            end = time.perf_counter()
            compile_time += end - start
            # Filter out duplicate rules
            rules = {
                sw_name: [
                    rule for rule in rules_list
                    if not _is_installed(sw_name, rule)
                ] for sw_name, rules_list in rules.items()
            }
            # Install rules
            start = time.perf_counter()
            num_rules += self.install_rules(rules)
            end = time.perf_counter()
            install_time += end - start
            # Remember the installed rules
            for sw_name, rules_list in rules.items():
                for rule in rules_list:
                    _add_installed_rule(sw_name, rule)

        print('Verification rules:', num_rules)
        print('Compile time:', compile_time, 'seconds')
        print('Install time:', install_time, 'seconds')

    def read_table_rules(self, switch_name):
        sw_conn = self.sw_conns[switch_name]
        print('\n----- Reading tables rules for %s -----' % sw_conn.name)
        for response in sw_conn.ReadTableEntries():
            for entity in response.entities:
                entry = entity.table_entry
                print('-----')
                table_name = self.p4info_helper.get_tables_name(entry.table_id)
                print('%s: ' % table_name, end=' ')
                for m in entry.match:
                    print(self.p4info_helper.get_match_field_name(
                        table_name, m.field_id),
                          end=' ')
                    print('%r' % (self.p4info_helper.get_match_field_value(m),),
                          end=' ')
                action = entry.action.action
                action_name = self.p4info_helper.get_actions_name(
                    action.action_id)
                print('->', action_name, end=' ')
                for p in action.params:
                    print(self.p4info_helper.get_action_param_name(
                        action_name, p.param_id),
                          end=' ')
                    print('%r' % p.value, end=' ')
                print()

    def process_packet_in(self, sw_conn, packet):
        invId = int.from_bytes(packet.metadata[0].value, byteorder='big')
        pkt_bytes = packet.payload
        eth = ethernet.Ethernet(pkt_bytes)
        print('Violation occurred at', sw_conn.name)
        print('Violated invariant:')
        print(self.invariants[invId])
        print(eth)

    def process_switch(self, sw_conn):
        try:
            while True:
                result = sw_conn.PacketIn()
                if result.WhichOneof('update') == 'packet':
                    self.process_packet_in(sw_conn, result.packet)
        except grpc.RpcError as e:
            printGrpcError(e)
        except Exception as e:
            print(e)

    async def async_start(self):
        try:
            self.set_reduced_MTU()
            self.connect()
            self.install_P4_programs()
            print('Installing forwarding rules')
            self.install_rules(self.network['rules'])
            print('Installing verification rules')
            self.install_verification_rules()
        except grpc.RpcError as e:
            printGrpcError(e)
            raise
        except Exception as e:
            print(e)
            raise

        print('===========================================')
        print('Start controller-switch event loops')
        print('===========================================')

        # Process each switch asynchronously
        loop = asyncio.get_running_loop()
        for sw_conn in self.sw_conns.values():
            self.futures.append(
                loop.run_in_executor(None, self.process_switch, sw_conn))

    def start(self):
        try:
            asyncio.run(self.async_start())
        except:
            print('Shutting down')
            self.shutdown()

    def shutdown(self):
        try:
            for future in self.futures:
                future.cancel()
            # ShutdownAllSwitchConnections()
        except:
            pass

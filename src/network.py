#!/usr/bin/env python3

import os
import json
import graphviz
import subprocess
from collections import defaultdict
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.cli import CLI

from .p4_mininet import P4Host
from .p4_mininet import P4Switch
from .p4_mininet import P4RuntimeSwitch


def run_mnexc_cmd(pid, cmd):
    fullcmd = 'mnexec -a {} {}'.format(pid, cmd)
    subprocess.run(fullcmd, shell=True, check=True)


def get_mac_addr(node, port):
    if issubclass(node.__class__, P4Switch):
        assert port >= 1
        mac = '08:00:{:02x}:{:02x}:{:02x}:{:02x}'.format(
            node.device_id // 256, node.device_id % 256, port // 256,
            port % 256)
        return mac
    elif isinstance(node, P4Host):
        assert port == None or port == 0
        mac = '08:00:00:00:{:02x}:{:02x}'.format(node.host_id // 256,
                                                 node.host_id % 256)
        return mac
    else:
        raise Exception('Unsupported node class {}'.format(node.__class__))


class Network:

    def __init__(self, output_dir, sw_path='simple_switch_grpc'):
        """ Initialize some attributes. Does not actually start the network.
            Use start() for that.
        """

        self.sw_path = sw_path
        self.out_dir = output_dir
        self.log_dir = os.path.join(output_dir, 'logs')
        self.pcap_dir = os.path.join(output_dir, 'pcaps')
        for dir_name in [self.log_dir, self.pcap_dir]:
            if not os.path.isdir(dir_name):
                if os.path.exists(dir_name):
                    raise Exception("'%s' is not a directory" % dir_name)
                os.makedirs(dir_name)

        self.net = Mininet(
            link=TCLink,
            host=P4Host,
            switch=P4RuntimeSwitch, # type: ignore
            controller=None, # type: ignore
            waitConnected=True)
        # note that Switch returns True (connected) by default

        # group name -> set{ switch names }
        self.groups = defaultdict(set)
        # switch name -> set{ json_str(table entries) }
        self.rules = defaultdict(set)

    def add_host(self, name, ip):
        return self.net.addHost(name, ip=ip)

    def add_switch(self, name):
        logFn = '%s/%s.log' % (self.log_dir, name)
        return self.net.addSwitch(name,
                                  sw_path=self.sw_path,
                                  pcap_dump=self.pcap_dir,
                                  log_file=logFn)

    # Note that switches start with port number 1, while other nodes start with 0.
    def add_link(self,
                 node1,
                 node2,
                 port1=None,
                 port2=None,
                 latency=None,
                 bandwidth=None):
        # Get MAC addrs for both ports
        addr1 = get_mac_addr(self.net.get(node1), port1)
        addr2 = get_mac_addr(self.net.get(node2), port2)

        return self.net.addLink(node1,
                                node2,
                                port1=port1,
                                port2=port2,
                                addr1=addr1,
                                addr2=addr2,
                                delay=latency,
                                bw=bandwidth)

    def add_group(self, sw_name, gr_name):
        self.groups[gr_name].add(sw_name)

    def add_rule(self, from_node, ip_prefix, to_node):
        rule = {'prefix': ip_prefix, 'next_hop': to_node}
        self.rules[from_node].add(json.dumps(rule, sort_keys=True))

    def get(self, name):
        return self.net.get(name)

    def staticArp(self):
        "Add all-pairs ARP entries to remove the need to handle broadcast."
        for src in self.net.hosts:
            for dst in self.net.hosts:
                if src != dst:
                    src.cmd('ip neigh add {} lladdr {} dev {}'.format(
                        dst.IP(), dst.MAC(), src.defaultIntf()))

    def setDefaultRoutes(self):
        "Set default routes on all hosts"
        for host in self.net.hosts:
            host.setDefaultRoute(host.defaultIntf())

    def export_network_json(self):
        data = {
            'hosts': {},
            'switches': {},
            'groups': {},
            'links': [],
            'rules': {}
        }

        for host in self.net.hosts:
            host_dict = {}
            host_dict['name'] = host.name
            host_dict['pid'] = host.pid
            host_dict['host_id'] = host.host_id
            host_dict['intfs'] = {}
            for intf in host.intfList():
                host_dict['intfs'][intf.name] = {
                    'name': intf.name,
                    'port': host.ports[intf],
                    'ip': intf.IP(),
                    'prefixLen': intf.prefixLen,
                    'mac': intf.MAC()
                }
            data['hosts'][host.name] = host_dict

        for sw in self.net.switches:
            sw_dict = {}
            sw_dict['name'] = sw.name
            sw_dict['pid'] = sw.pid
            sw_dict['device_id'] = sw.device_id
            sw_dict['grpc_port'] = sw.grpc_port
            sw_dict['host_ports'] = []
            sw_dict['intfs'] = {}
            for intf in sw.intfList():
                sw_dict['intfs'][intf.name] = {
                    'name': intf.name,
                    'port': sw.ports[intf],
                    'ip': intf.IP(),
                    'prefixLen': intf.prefixLen,
                    'mac': intf.MAC()
                }
            data['switches'][sw.name] = sw_dict

        for gr_name, devices in self.groups.items():
            data['groups'][gr_name] = list(devices)

        for link in self.net.links:
            node1 = link.intf1.node
            node2 = link.intf2.node
            data['links'].append({
                'node1': node1.name,
                'node2': node2.name,
                'port1': node1.ports[link.intf1],
                'port2': node2.ports[link.intf2],
                'intf1': link.intf1.name,
                'intf2': link.intf2.name
            })
            if isinstance(node1, self.net.host):
                data['switches'][node2.name]['host_ports'].append(
                    node2.ports[link.intf2])
            if isinstance(node2, self.net.host):
                data['switches'][node1.name]['host_ports'].append(
                    node1.ports[link.intf1])
            node1_dev_type = ('hosts' if isinstance(node1, self.net.host) else
                              'switches')
            node2_dev_type = ('hosts' if isinstance(node2, self.net.host) else
                              'switches')
            data[node1_dev_type][node1.name]['intfs'][
                link.intf1.name]['neighborNode'] = node2.name
            data[node1_dev_type][node1.name]['intfs'][
                link.intf1.name]['neighborPort'] = node2.ports[link.intf2]
            data[node2_dev_type][node2.name]['intfs'][
                link.intf2.name]['neighborNode'] = node1.name
            data[node2_dev_type][node2.name]['intfs'][
                link.intf2.name]['neighborPort'] = node1.ports[link.intf1]

        # Translate: switch name -> set{ json_str(table entries) }
        # To: switch name -> list( table entries )
        for sw_name, rule_set in self.rules.items():
            data['rules'][sw_name] = list()
            for rule_str in rule_set:
                data['rules'][sw_name].append(json.loads(rule_str))

        fn = os.path.join(self.out_dir, 'network.json')
        with open(fn, 'w') as outfile:
            json.dump(data, outfile, indent=4)

    def export_network_dot(self):
        fn = os.path.join(self.out_dir, 'network.dot')
        dot = graphviz.Graph(
            'network',
            filename=fn,
            format='png',
            graph_attr={
                'nodesep': '1' # inches
            },
            node_attr={
                'margin': '0.2', # inches
                'fontname': 'Iosevka'
            },
            edge_attr={
                'minlen': '5', # inches
                'labeldistance': '5', # multiplicative scaling
                'fontname': 'Iosevka'
            })
        for host in self.net.hosts:
            dot.node(name=host.name,
                     label=host.name + ' (' + str(host.host_id) + ')',
                     shape='circle')
        for sw in self.net.switches:
            dot.node(name=sw.name,
                     label=sw.name + ' (' + str(sw.device_id) + ')',
                     shape='box')
        for link in self.net.links:
            taillabel = link.intf1.name + ' (' + str(
                link.intf1.node.ports[link.intf1]) + ')'
            if link.intf1.IP():
                taillabel += '\n' + link.intf1.IP() + '/' + str(
                    link.intf1.prefixLen)
            taillabel += '\n' + link.intf1.MAC()
            headlabel = link.intf2.name + ' (' + str(
                link.intf2.node.ports[link.intf2]) + ')'
            if link.intf2.IP():
                headlabel += '\n' + link.intf2.IP() + '/' + str(
                    link.intf2.prefixLen)
            headlabel += '\n' + link.intf2.MAC()
            dot.edge(tail_name=link.intf1.node.name,
                     head_name=link.intf2.node.name,
                     taillabel=taillabel,
                     headlabel=headlabel)
        dot.render()

    def start(self):
        # start the Mininet instance
        self.net.start()

        self.staticArp()
        self.setDefaultRoutes()
        self.export_network_json()
        self.export_network_dot()

    def stop(self):
        self.net.stop()

    def run_cli(self):
        for s in self.net.switches:
            s.describe()
        for h in self.net.hosts:
            h.describe()
        CLI(self.net)

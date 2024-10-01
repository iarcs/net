# Copyright 2013-present Barefoot Networks, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import os
import psutil
import tempfile
from sys import exit
from time import sleep
from mininet.node import Switch, Host
from mininet.log import setLogLevel, info, error, debug
from mininet.moduledeps import pathCheck

from .constants import CPU_PORT
from .constants import SWITCH_START_TIMEOUT


def check_listening_on_port(port):
    for c in psutil.net_connections(kind='inet'):
        if c.status == 'LISTEN' and c.laddr and c.laddr[1] == port:
            return True
    return False


class P4Host(Host): # pyright: reportOptionalMemberAccess=false
    host_id = 1

    def __init__(self, name, **kwargs):
        Host.__init__(self, name, **kwargs)
        self.host_id = P4Host.host_id
        P4Host.host_id += 1

    def config(self, **params):
        r = super(Host, self).config(**params)

        for off in ["rx", "tx", "sg"]:
            cmd = "/usr/bin/ethtool --offload %s %s off" % (
                self.defaultIntf().name, off)
            self.cmd(cmd)

        # disable IPv6
        self.cmd("sysctl -w net.ipv6.conf.all.disable_ipv6=1")
        self.cmd("sysctl -w net.ipv6.conf.default.disable_ipv6=1")
        self.cmd("sysctl -w net.ipv6.conf.lo.disable_ipv6=1")

        return r

    def describe(self):
        print("**********")
        print(self.name)
        print("default interface: %s\t%s\t%s" %
              (self.defaultIntf().name, self.defaultIntf().IP(),
               self.defaultIntf().MAC()))
        print("**********")


class P4Switch(Switch):
    """P4 virtual switch"""
    device_id = 1
    next_thrift_port = 9090

    def __init__(self,
                 name,
                 sw_path=None,
                 json_path=None,
                 thrift_port=None,
                 pcap_dump=None,
                 log_console=False,
                 log_file=None,
                 verbose=False,
                 device_id=None,
                 enable_debugger=False,
                 enable_nanolog=False,
                 **kwargs):
        Switch.__init__(self, name, **kwargs)
        assert (sw_path)
        # make sure that the provided sw_path is valid
        pathCheck(sw_path)
        # make sure that the provided JSON file exists
        if json_path and not os.path.isfile(json_path):
            error("Invalid JSON file: {}\n".format(json_path))
            exit(1)
        self.sw_path = sw_path
        self.json_path = json_path
        if thrift_port is not None:
            self.thrift_port = thrift_port
        else:
            self.thrift_port = P4Switch.next_thrift_port
            P4Switch.next_thrift_port += 1
        if check_listening_on_port(self.thrift_port):
            error('%s: port %d is bound by another process\n' %
                  (self.name, self.thrift_port))
            exit(1)
        self.pcap_dump = pcap_dump
        self.log_console = log_console
        self.log_file = log_file
        self.verbose = verbose
        if device_id is not None:
            self.device_id = device_id
            P4Switch.device_id = max(P4Switch.device_id, device_id + 1)
        else:
            self.device_id = P4Switch.device_id
            P4Switch.device_id += 1
        self.enable_debugger = enable_debugger
        if enable_nanolog:
            self.nanomsg = "ipc:///tmp/bm-{}-log.ipc".format(self.device_id)
        else:
            self.nanomsg = None

    @classmethod
    def setup(cls):
        pass

    def check_switch_started(self, pid):
        """While the process is running (pid exists), we check if the Thrift
        server has been started. If the Thrift server is ready, we assume that
        the switch was started successfully. This is only reliable if the Thrift
        server is started at the end of the init process"""
        while True:
            if not os.path.exists(os.path.join("/proc", str(pid))):
                return False
            if check_listening_on_port(self.thrift_port):
                return True
            sleep(0.5)

    def formulate_cmd(self):
        cmd = [self.sw_path]
        for port, intf in list(self.intfs.items()):
            if not intf.IP():
                cmd.extend(['-i', str(port) + '@' + intf.name])
        if self.pcap_dump:
            cmd.append('--pcap %s' % self.pcap_dump)
        if self.thrift_port:
            cmd.extend(['--thrift-port', str(self.thrift_port)])
        if self.nanomsg:
            cmd.extend(['--nanolog', self.nanomsg])
        cmd.extend(['--device-id', str(self.device_id)])
        if self.json_path:
            cmd.append(self.json_path)
        else:
            cmd.append('--no-p4')
        if self.enable_debugger:
            cmd.append('--debugger')
        if self.log_file or self.log_console:
            cmd.append('--log-console')
        if self.verbose:
            cmd.append('--log-level debug')
        else:
            cmd.append('--log-level info')
        return cmd

    def start(self, controllers):
        info("Starting P4 switch {}.\n".format(self.name))
        cmd = ' '.join(self.formulate_cmd())
        if self.log_file:
            cmd += ' >' + self.log_file + ' 2>&1'
        info(cmd + '\n')

        pid = None
        with tempfile.NamedTemporaryFile() as f:
            self.cmd(cmd + ' & echo $! >> ' + f.name)
            pid = int(f.read())
        debug("P4 switch {} PID is {}.\n".format(self.name, pid))
        if not self.check_switch_started(pid):
            error("P4 switch {} did not start correctly.\n".format(self.name))
            exit(1)
        info("P4 switch {} has been started.\n".format(self.name))

    def stop(self):
        "Terminate P4 switch."
        self.cmd('kill %' + self.sw_path)
        self.cmd('wait')
        self.deleteIntfs()

    def attach(self, intf):
        "Connect a data port"
        assert (0)

    def detach(self, intf):
        "Disconnect a data port"
        assert (0)


class P4RuntimeSwitch(P4Switch):
    "BMv2 switch with gRPC support"
    next_grpc_port = 50051
    # default packet-in / packet-out port usage
    cpu_port = CPU_PORT

    def __init__(self,
                 name,
                 sw_path=None,
                 json_path=None,
                 grpc_port=None,
                 thrift_port=None,
                 pcap_dump=None,
                 log_console=False,
                 log_file=None,
                 verbose=False,
                 device_id=None,
                 enable_debugger=False,
                 enable_nanolog=False,
                 **kwargs):
        P4Switch.__init__(self,
                          name,
                          sw_path=sw_path,
                          json_path=json_path,
                          thrift_port=thrift_port,
                          pcap_dump=pcap_dump,
                          log_console=log_console,
                          log_file=log_file,
                          verbose=verbose,
                          device_id=device_id,
                          enable_debugger=enable_debugger,
                          enable_nanolog=enable_nanolog,
                          **kwargs)
        if grpc_port is not None:
            self.grpc_port = grpc_port
        else:
            self.grpc_port = P4RuntimeSwitch.next_grpc_port
            P4RuntimeSwitch.next_grpc_port += 1

        if check_listening_on_port(self.grpc_port):
            error('%s: port %d is bound by another process\n' %
                  (self.name, self.grpc_port))
            exit(1)

        self.cpu_port = P4RuntimeSwitch.cpu_port

    def describe(self):
        print("%s -> gRPC port: %d" % (self.name, self.grpc_port))

    def check_switch_started(self, pid):
        for _ in range(SWITCH_START_TIMEOUT * 2):
            if not os.path.exists(os.path.join("/proc", str(pid))):
                return False
            if check_listening_on_port(self.grpc_port):
                return True
            sleep(0.5)
        return False

    def formulate_cmd(self):
        cmd = super().formulate_cmd()
        cmd.append('--')
        if self.grpc_port:
            cmd.append('--grpc-server-addr 0.0.0.0:' + str(self.grpc_port))
        if self.cpu_port:
            cmd.append('--cpu-port ' + str(self.cpu_port))
        return cmd

    def start(self, controllers):
        info("Starting P4 switch {}.\n".format(self.name))
        cmd = ' '.join(self.formulate_cmd())
        if self.log_file:
            cmd += ' >' + self.log_file + ' 2>&1'
        info(cmd + '\n')

        pid = None
        with tempfile.NamedTemporaryFile() as f:
            self.cmd(cmd + ' & echo $! >> ' + f.name)
            pid = int(f.read())
        debug("P4 switch {} PID is {}.\n".format(self.name, pid))
        debug("CPU port is {}.\n".format(self.cpu_port))
        if not self.check_switch_started(pid):
            error("P4 switch {} did not start correctly.\n".format(self.name))
            exit(1)
        info("P4 switch {} has been started.\n".format(self.name))

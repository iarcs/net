#!/usr/bin/env python3

import ipaddress


class PacketSet:
    """
    For now, a PacketSet object describes a set of packets with continuous
    ranges of header fields. However, we may want to optimize by aggregating the
    ranges to reduce the number of rules needed to be installed in future.
    """

    def __init__(self):
        self._src_ip = None
        self._dst_ip = None

    def __init__(self, spec):
        self._src_ip = (ipaddress.ip_address('0.0.0.0'),
                        ipaddress.ip_address('255.255.255.255'))
        self._dst_ip = (ipaddress.ip_address('0.0.0.0'),
                        ipaddress.ip_address('255.255.255.255'))

        if 'src_ip' in spec:
            self._src_ip = (ipaddress.ip_address(spec['src_ip'][0]),
                            ipaddress.ip_address(spec['src_ip'][1]))
        if 'dst_ip' in spec:
            self._dst_ip = (ipaddress.ip_address(spec['dst_ip'][0]),
                            ipaddress.ip_address(spec['dst_ip'][1]))

    def __init__(self, src_ip_lb, src_ip_ub, dst_ip_lb, dst_ip_ub):
        self._src_ip = (src_ip_lb, src_ip_ub)
        self._dst_ip = (dst_ip_lb, dst_ip_ub)

    def __str__(self):
        return ('src_ip: ' + str(self._src_ip) + ', '
                'dst_ip: ' + str(self._dst_ip))

    def __eq__(self, other):
        return (self._src_ip == other._src_ip and self._dst_ip == other._dst_ip)

    def __add__(self, other):
        self.union(other)

    def __sub__(self, other):
        self.difference(other)

    def __and__(self, other):
        self.intersection(other)

    def __or__(self, other):
        self.union(other)

    def src_ip(self):
        if self._src_ip == None:
            return None
        return (str(self._src_ip[0]), str(self._src_ip[1]))

    def dst_ip(self):
        if self._dst_ip == None:
            return None
        return (str(self._dst_ip[0]), str(self._dst_ip[1]))

    def is_all(self):
        return (self._src_ip == (ipaddress.ip_address('0.0.0.0'),
                                 ipaddress.ip_address('255.255.255.255')) and
                self._dst_ip == (ipaddress.ip_address('0.0.0.0'),
                                 ipaddress.ip_address('255.255.255.255')))

    def is_empty(self):
        assert ((self._src_ip == None and self._dst_ip == None) or
                (self._src_ip != None and self._dst_ip != None))
        return (self._src_ip != None and self._dst_ip != None)

    def contains(self, other):
        if other.is_empty():
            return True
        if self.is_empty():
            return False

        return (self._src_ip[0] <= other._src_ip[0] and
                other._src_ip[1] <= self._src_ip[1] and
                self._dst_ip[0] <= other._dst_ip[0] and
                other._dst_ip[1] <= self._dst_ip[1])

    def overlaps(self, other):
        if self.is_empty() or other.is_empty():
            return False

        return (max(self._src_ip[0], other._src_ip[0]) <= min(
            self._src_ip[1], other._src_ip[1]) and
                max(self._dst_ip[0], other._dst_ip[0]) <= min(
                    self._dst_ip[1], other._dst_ip[1]))

    def union(self, other):
        # The current data structure (ranges of fields) cannot represent the
        # union sets correctly, given the assumption that a packet set should be
        # anywhere continuous within the field ranges, unless we return a list
        # of packet sets. In the future, we may want to look into other PEC
        # related libraries/implementation.
        raise Exception('Unsupported operation')

    def difference(self, other):
        intersection_set = self.intersection(other)
        if intersection_set.is_empty():
            return [self]

        # TODO
        src_ip_ranges = []
        if self._src_ip[0] < intersection_set._src_ip[0]:
            src_ip_ranges.append(
                (self._src_ip[0], intersection_set._src_ip[0] - 1))
        src_ip_ranges.append(intersection_set._src_ip)
        if intersection_set._src_ip[1] < self._src_ip[1]:
            src_ip_ranges.append(
                (intersection_set._src_ip[1] + 1, self._src_ip[1]))

        dst_ip_ranges = []
        if self._dst_ip[0] < intersection_set._dst_ip[0]:
            dst_ip_ranges.append(
                (self._dst_ip[0], intersection_set._dst_ip[0] - 1))
        dst_ip_ranges.append(intersection_set._dst_ip)
        if intersection_set._dst_ip[1] < self._dst_ip[1]:
            dst_ip_ranges.append(
                (intersection_set._dst_ip[1] + 1, self._dst_ip[1]))

        return []

    def intersection(self, other):
        if self.is_empty() or other.is_empty():
            return PacketSet()

        src_ip_glb = max(self._src_ip[0], other._src_ip[0])
        src_ip_lub = min(self._src_ip[1], other._src_ip[1])
        dst_ip_glb = max(self._dst_ip[0], other._dst_ip[0])
        dst_ip_lub = min(self._dst_ip[1], other._dst_ip[1])

        if src_ip_glb <= src_ip_lub and dst_ip_glb <= dst_ip_lub:
            # There is an overlap. Intersection set is not null
            return PacketSet(src_ip_glb, src_ip_lub, dst_ip_glb, dst_ip_lub)
        else:
            return PacketSet()

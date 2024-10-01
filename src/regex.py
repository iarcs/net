#!/usr/bin/env python3

from collections import defaultdict
import re


class Node:

    def __init__(self, val, pos, left=None, right=None):
        self.val = val
        self.pos = pos
        self.left = left
        self.right = right

    def __str__(self):
        return str(self.val)

    def is_leaf(self):
        return self.left == None and self.right == None

    def nullable(self):
        if self.is_leaf() and self.val == Regex.OP_END:
            return True
        elif self.is_leaf() and self.pos > 0:
            return False
        elif self.val == Regex.OP_UNION:
            return self.left.nullable() or self.right.nullable()
        elif self.val == Regex.OP_CONCAT:
            return self.left.nullable() and self.right.nullable()
        elif self.val == Regex.OP_KLEENE:
            return True
        else:
            raise Exception('Unrecognizable node ' + str(self))


class AST:

    def __init__(self, postfix, alphabet):
        stack = list()
        pos = 1
        for token in postfix:
            if token in alphabet.union(set([Regex.OP_END])):
                stack.append(Node(token, pos))
                pos += 1
            elif token in (Regex.OP_CONCAT, Regex.OP_UNION):
                r = stack.pop()
                l = stack.pop()
                stack.append(Node(token, -1, l, r))
            elif token == Regex.OP_KLEENE:
                l = stack.pop()
                stack.append(Node(token, -1, l))
            else:
                raise Exception('Invalid token: ' + token)
        self.root = stack[0]

    @staticmethod
    def _get_positions(node, firstPositions, lastPositions, followPositions):
        if node == None:
            return

        # Post-order traversal for bottom-up construction
        AST._get_positions(node.left, firstPositions, lastPositions,
                           followPositions)
        AST._get_positions(node.right, firstPositions, lastPositions,
                           followPositions)

        # First positions
        if node.is_leaf() and node.pos > 0:
            firstPositions[node] = set([node.pos])
        elif node.val == Regex.OP_UNION:
            firstPositions[node] = (firstPositions[node.left] |
                                    firstPositions[node.right])
        elif node.val == Regex.OP_CONCAT:
            if node.left.nullable():
                firstPositions[node] = (firstPositions[node.left] |
                                        firstPositions[node.right])
            else:
                firstPositions[node] = firstPositions[node.left]
        elif node.val == Regex.OP_KLEENE:
            firstPositions[node] = firstPositions[node.left]
        else:
            firstPositions[node] = set()

        # Last positions
        if node.is_leaf() and node.pos > 0:
            lastPositions[node] = set([node.pos])
        elif node.val == Regex.OP_UNION:
            lastPositions[node] = (lastPositions[node.left] |
                                   lastPositions[node.right])
        elif node.val == Regex.OP_CONCAT:
            if node.right.nullable():
                lastPositions[node] = (lastPositions[node.left] |
                                       lastPositions[node.right])
            else:
                lastPositions[node] = lastPositions[node.right]
        elif node.val == Regex.OP_KLEENE:
            lastPositions[node] = lastPositions[node.left]
        else:
            lastPositions[node] = set()

        # Follow positions
        if node.val == Regex.OP_CONCAT:
            for i in lastPositions[node.left]:
                followPositions[i] |= firstPositions[node.right]
        elif node.val == Regex.OP_KLEENE:
            for i in lastPositions[node]:
                followPositions[i] |= firstPositions[node]

    def getPositions(self):
        firstpos = dict()
        lastpos = dict()
        followpos = defaultdict(set)
        AST._get_positions(self.root, firstpos, lastpos, followpos)
        return (firstpos, lastpos, followpos)

    @staticmethod
    def _getTokenPosMap(node, tokenPosMap):
        if node == None:
            return

        AST._getTokenPosMap(node.left, tokenPosMap)
        AST._getTokenPosMap(node.right, tokenPosMap)

        if node.pos >= 0:
            tokenPosMap[node.val].add(node.pos)

    def getTokenPosMap(self):
        tokenPosMap = defaultdict(set)
        AST._getTokenPosMap(self.root, tokenPosMap)
        return tokenPosMap


class Regex():
    '''
    Regular expression class.
    The alphabet set is obtained from the network topology.
    Other non-terminal tokens:
        '*': Kleene closure
        '&': Concatenation
        '|': Union
        '(', ')': Prioritization
    '''

    OP_KLEENE = '*'
    OP_CONCAT = '&'
    OP_UNION = '|'
    OP_END = '#'
    priority = {OP_KLEENE: 3, OP_CONCAT: 2, OP_UNION: 1, OP_END: 0}
    non_terminals = set([OP_KLEENE, OP_CONCAT, OP_UNION, OP_END, '(', ')'])

    def __init__(self, pattern, network):
        alphabet, drop_alph = Regex.compute_alphabet(network)
        pattern = Regex.preprocessor(pattern, alphabet, network)
        tokens = Regex.lexer(pattern, alphabet, drop_alph)
        self.ast = Regex.parser(tokens, alphabet, drop_alph)

    @staticmethod
    def compute_alphabet(network):
        alphabet = set()
        for link in network['links']:
            if link['node1'] not in network['hosts']:
                alphabet.add(link['node1'] + 'p' + str(link['port1']))
            if link['node2'] not in network['hosts']:
                alphabet.add(link['node2'] + 'p' + str(link['port2']))
        drop_alph = set()
        for sw_name in network['switches'].keys():
            drop_alph.add(sw_name + 'd')
        return alphabet, drop_alph

    @staticmethod
    def preprocessor(pattern, alphabet, network):
        pattern = pattern.replace(' ', '')

        def get_union_string(tokens):
            string = '('
            for token in tokens:
                string += (token + Regex.OP_UNION)
            string = string[:-1] + ')'
            return string

        while True:
            i = pattern.find('[^')
            if i == -1:
                break
            j = pattern.find(']', i)
            tokens = Regex.lexer(pattern[i + 2:j], alphabet)
            pattern = (pattern[:i] + get_union_string(alphabet - set(tokens)) +
                       pattern[j + 1:])

        while True:
            i = pattern.find('[')
            if i == -1:
                break
            j = pattern.find(']', i)
            tokens = Regex.lexer(pattern[i + 1:j], alphabet)
            pattern = (pattern[:i] + get_union_string(set(tokens)) +
                       pattern[j + 1:])

        while True:
            i = pattern.find('.')
            if i == -1:
                break
            pattern = pattern[:i] + get_union_string(alphabet) + pattern[i + 1:]

        while True:
            i = pattern.find('out')
            if i == -1:
                break
            outbound = set()
            for sw_name, sw in network['switches'].items():
                for hport in sw['host_ports']:
                    outbound.add(sw_name + 'p' + str(hport))
            pattern = pattern[:i] + get_union_string(outbound) + pattern[i + 3:]

        def collect_dev_endpoints(device, network):
            # If the device is a host, or if the device is a switch whose
            # interface in question is connected to another switch, then
            # collect the neighboring interfaces as endpoints. Otherwise, if
            # the device is a switch whose interface in question is
            # connected to a host, then collect the switch's (entry)
            # interface as an endpoint.
            endpoints = set()
            if device in network['hosts']:
                dev_type = 'hosts'
            elif device in network['switches']:
                dev_type = 'switches'
            else:
                raise Exception('Unknown device ' + device)
            intfs_dict = network[dev_type][device]['intfs']
            for intf in intfs_dict.values():
                if 'neighborNode' not in intf:
                    continue
                if (dev_type == 'hosts' or
                        intf['neighborNode'] in network['switches']):
                    endpoints.add(intf['neighborNode'] + 'p' +
                                  str(intf['neighborPort']))
                else:
                    endpoints.add(device + 'p' + str(intf['port']))
            return endpoints

        # Devices without ports
        for device in (list(network['hosts'].keys()) +
                       list(network['switches'].keys())):
            while True:
                m = re.search('({})[^p]'.format(device),
                              pattern,
                              flags=re.IGNORECASE)
                if m == None:
                    break
                endpoints = collect_dev_endpoints(device, network)
                pattern = (pattern[:m.start(1)] + get_union_string(endpoints) +
                           pattern[m.end(1):])

        # Custom groups
        for gr_name, devices in network['groups'].items():
            while True:
                i = pattern.find(gr_name)
                if i == -1:
                    break
                endpoints = set()
                for device in devices:
                    endpoints |= collect_dev_endpoints(device, network)
                pattern = (pattern[:i] + get_union_string(endpoints) +
                           pattern[i + len(gr_name):])

        return pattern + Regex.OP_END

    @staticmethod
    def lexer(pattern, alphabet, drop_alph):
        # Lexical analysis
        tokens = list()
        ep_pattern = re.compile(r's\d+(p\d+|d)', flags=re.IGNORECASE)

        i = 0
        while i < len(pattern):
            if pattern[i] in Regex.non_terminals:
                tokens.append(pattern[i])
                i += 1
            else:
                m = ep_pattern.match(pattern[i:])
                if m is None or m.end() == -1:
                    raise Exception('Invalid pattern: ' + pattern[i:])
                endpoint = m.group(0)
                if endpoint not in alphabet.union(drop_alph):
                    raise Exception(
                        'Endpoint does not exist or is not connected: ' +
                        endpoint)
                tokens.append(endpoint)
                i += m.end()

        return tokens

    @staticmethod
    def parser(tokens, alphabet, drop_alph):
        # Syntax analysis
        all_alph = alphabet.union(drop_alph)

        # Add concatenation symbols
        prev_token = None
        new_tokens = list()
        for token in tokens:
            if len(token) > 0:
                if (prev_token in all_alph.union(set([Regex.OP_KLEENE, ')']))
                        and token in all_alph.union(set([Regex.OP_END, '(']))):
                    new_tokens.append(Regex.OP_CONCAT)
            new_tokens.append(token)
            prev_token = token
        tokens = new_tokens
        del new_tokens

        # Postfix order (Shunting yard algorithm)
        postfix = list()
        op_stack = list()
        for token in tokens:
            if token in all_alph.union(set([Regex.OP_END])):
                postfix.append(token)
            elif token == '(':
                op_stack.append(token)
            elif token == ')':
                op = op_stack.pop()
                while op != '(':
                    postfix.append(op)
                    op = op_stack.pop()
            else:
                if len(op_stack) == 0:
                    op_stack.append(token)
                else:
                    op = op_stack[-1]
                    while (op != '(' and
                           Regex.priority[op] >= Regex.priority[token]):
                        postfix.append(op_stack.pop())
                        if len(op_stack) == 0:
                            break
                        op = op_stack[-1]
                    op_stack.append(token)
        while len(op_stack) > 0:
            postfix.append(op_stack.pop())

        return AST(postfix, all_alph)

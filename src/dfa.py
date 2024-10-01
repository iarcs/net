#!/usr/bin/env python3

from collections import defaultdict, deque
import graphviz

from .regex import Regex


class DFA:

    def __init__(self, regex, network, fn='dfa'):
        self.states = set()
        self.initial = None
        self.accepting = set()
        self.transitions = defaultdict(dict)
        self._construct(regex.ast, network)
        self._simplify_states()
        self.export_transition_diagram(fn + '.orig')
        self._topo_constraints_opt(network)
        self.export_transition_diagram(fn)

    def _construct(self, ast, network):
        alphabet, drop_alph = Regex.compute_alphabet(network)
        firstpos, _, followpos = ast.getPositions()
        tokenPosMap = ast.getTokenPosMap()

        self.initial = frozenset(firstpos[ast.root])
        self.states.add(self.initial)
        q = deque()
        q.append(self.initial)

        while len(q) > 0:
            cur_state = q.popleft()
            for a in alphabet.union(drop_alph):
                nextstate = set()
                matchedPositions = cur_state & tokenPosMap[a]
                for pos in matchedPositions:
                    nextstate |= followpos[pos]
                nextstate = frozenset(nextstate)
                # This if statement removes all the invalid input transitions
                if len(nextstate) == 0:
                    continue
                if nextstate not in self.states:
                    self.states.add(nextstate)
                    q.append(nextstate)
                self.transitions[cur_state][a] = nextstate

        assert (len(tokenPosMap[Regex.OP_END]) == 1)
        for state in self.states:
            if tokenPosMap[Regex.OP_END].issubset(state):
                self.accepting.add(state)

    def _simplify_states(self):
        remap = dict()
        newstates = set()
        newinitial = None
        newaccepting = set()
        newtransitions = defaultdict(dict)

        def __preorder_remap(state, i):
            remap[state] = i
            newstates.add(i)
            i += 1
            for next_state in self.transitions[state].values():
                if next_state not in remap:
                    __preorder_remap(next_state, i)

        __preorder_remap(self.initial, 0)

        newinitial = remap[self.initial]
        for state, trans in self.transitions.items():
            for input_, nextstate in trans.items():
                newtransitions[remap[state]][input_] = remap[nextstate]
        for state in self.accepting:
            newaccepting.add(remap[state])

        self.states = newstates
        self.initial = newinitial
        self.accepting = newaccepting
        self.transitions = dict(newtransitions)

    def _topo_constraints_opt(self, network):

        def __gather_intfs(state, sym_loc, network):
            sym_intfs = set()
            for loc in sym_loc:
                if loc in network['hosts']:
                    # Only the initial state can enter the network
                    if state == self.initial:
                        for intf in network['hosts'][loc]['intfs'].values():
                            if 'neighborNode' not in intf:
                                continue
                            sym_intfs.add(intf['neighborNode'] + 'p' +
                                          str(intf['neighborPort']))
                elif loc in network['switches']:
                    for intf in network['switches'][loc]['intfs'].values():
                        if 'neighborNode' not in intf:
                            continue
                        sym_intfs.add(loc + 'p' + str(intf['port']))
                else:
                    raise Exception('Unknown device ' + loc)
            return sym_intfs

        def __process_next_state(q, visited_states, invalid_inputs, network):
            cur_state, sym_loc = q.popleft()
            next_states = defaultdict(set)

            if cur_state not in visited_states:
                visited_states[cur_state] = sym_loc
            elif not sym_loc.issubset(visited_states[cur_state]):
                visited_states[cur_state] |= sym_loc
                sym_loc = visited_states[cur_state]
            else:
                return

            if cur_state not in self.transitions:
                return

            # Gather all valid interfaces for sym_loc
            sym_intfs = __gather_intfs(cur_state, sym_loc, network)
            invalid_inputs[cur_state] = set()

            for input_, next_state in self.transitions[cur_state].items():
                # Check if input_ is plausible given the current sym_loc
                if input_ not in sym_intfs:
                    invalid_inputs[cur_state].add(input_)
                    continue

                egress_node = input_[:input_.rfind('p')]
                egress_port = input_[input_.rfind('p') + 1:]
                egress_intf = egress_node + '-eth' + egress_port
                intf = network['switches'][egress_node]['intfs'][egress_intf]
                if 'neighborNode' not in intf:
                    invalid_inputs[cur_state].add(input_)
                    continue

                neighbor_node = intf['neighborNode']
                if (neighbor_node in network['hosts'] and
                        neighbor_node in sym_loc):
                    next_states[next_state].add(egress_node)
                if egress_node in sym_loc:
                    next_states[next_state].add(neighbor_node)

            # Push all (next_state, next_sym_loc) entries to the queue
            for next_state, next_sym_loc in next_states.items():
                q.append((next_state, next_sym_loc))

        q = deque()
        q.append((self.initial, set(network['hosts'].keys())))
        visited_states = dict() # state -> symbolic location (set())
        invalid_inputs = dict() # state -> invalid inputs (set())
        while len(q) > 0:
            __process_next_state(q, visited_states, invalid_inputs, network)

        # Delete the invalid inputs after reaching the fixed point
        for state, inputs in invalid_inputs.items():
            for input_ in inputs:
                del self.transitions[state][input_]

    def dump(self):
        print("States: " + str(self.states))
        print("Initial state: " + str(self.initial))
        print("Accepting states: " + str(self.accepting))
        print("Transitions: ")
        for i in self.transitions:
            print(str(i) + ": " + str(self.transitions[i]))

    def export_transition_diagram(self, fn='dfa'):
        dot = graphviz.Digraph(
            'DFA',
            filename=fn + '.dot',
            format='png',
            graph_attr={
                'nodesep': '0.4' # inches
            },
            node_attr={'fontname': 'Iosevka'},
            edge_attr={
                'minlen': '1', # inches
                'labeldistance': '5', # multiplicative scaling
                'fontname': 'Iosevka'
            })
        for state in self.states:
            if state in self.accepting:
                dot.node(name=str(state),
                         label=str(state),
                         shape='doublecircle')
            else:
                dot.node(name=str(state), label=str(state), shape='circle')
        for curr_state, trans in self.transitions.items():
            for input_, next_state in trans.items():
                dot.edge(tail_name=str(curr_state),
                         head_name=str(next_state),
                         label=input_)
        dot.render()

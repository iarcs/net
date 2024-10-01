#!/usr/bin/env python3

import argparse
from src.controller import Controller


def main():
    # Parse arguments
    parser = argparse.ArgumentParser(description='P4Runtime controller')
    parser.add_argument('--network',
                        help='network.json from our mininet',
                        type=str,
                        action='store',
                        required=False,
                        default='network.json')
    parser.add_argument('--invariants',
                        help='invariants specification',
                        type=str,
                        action='store',
                        required=False,
                        default='invariants.json')
    parser.add_argument('--p4info',
                        help='p4info proto in text format from p4c',
                        type=str,
                        action='store',
                        required=False,
                        default='build/p4info.txt')
    parser.add_argument('--bmv2-json',
                        help='BMv2 JSON file from p4c',
                        type=str,
                        action='store',
                        required=False,
                        default='build/switch.json')
    args = parser.parse_args()

    # Start the controller
    controller = Controller(network_json=args.network,
                            inv_json=args.invariants,
                            p4info_path=args.p4info,
                            bmv2_json=args.bmv2_json)
    controller.start()


if __name__ == '__main__':
    main()

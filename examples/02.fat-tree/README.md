## Fat-tree topology

To run the experiment, first start the network by:
```sh
$ make
# or
$ python run_network.py --ecmp
```

And then in another terminal prompt, run:
```sh
$ make run
```
This will launch the controller which installs the rules to the P4 switches.

Please note that the specifications in `invariants.json` are intended for a
4-ary fat-tree. If a different arity is used (e.g., `python run_network.py
--ecmp -k 6`), you may want to modify the invariants spec as well.

OUTPUT_DIR = output
SRC_DIR = ../../src
BUILD_DIR = ../../build
.DEFAULT_GOAL = network

network:
	sudo python3 run_network.py

network-ecmp:
	sudo python3 run_network.py --ecmp

run: $(BUILD_DIR)/controller $(BUILD_DIR)/switch.json $(BUILD_DIR)/p4info.txt $(OUTPUT_DIR)/network.json
	sudo $< \
		--output $(OUTPUT_DIR) \
		--network $(OUTPUT_DIR)/network.json \
		--invariants invariants.json \
		--p4 $(BUILD_DIR)/switch.json \
		--p4info $(BUILD_DIR)/p4info.txt

stop:
	sudo mn -c

clean: stop
	sudo rm -rf $(OUTPUT_DIR)

.PHONY: network network-ecmp run stop clean

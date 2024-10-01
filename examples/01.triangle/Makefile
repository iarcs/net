BUILD_DIR = build
SRC_DIR = ../../src
.DEFAULT_GOAL := network

network:
	sudo python3 run_network.py

run: $(BUILD_DIR)/switch.json network.json
	sudo python3 ../../main.py \
		--network network.json \
		--invariants invariants.json \
		--p4info "$(BUILD_DIR)/p4info.txt" \
		--bmv2-json "$<"

$(BUILD_DIR)/%.json: $(SRC_DIR)/%.p4
	mkdir -p $(BUILD_DIR)
	p4c --target bmv2 \
		--arch v1model \
		--std p4-16 \
		--p4runtime-files $(BUILD_DIR)/p4info.txt \
		-o $(BUILD_DIR) \
		$<

stop:
	sudo mn -c

clean: stop
	rm -rf $(BUILD_DIR) network.json *.dot *.dot.*
	sudo rm -rf pcaps logs

.PHONY: network network-ecmp run stop clean

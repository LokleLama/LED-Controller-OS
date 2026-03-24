
.PHONY: test
test:
	mkdir -p build-test
	cd build-test && \
	cmake -DBUILD_TESTS=ON .. && \
	make -j$(nproc) && \
	./test/test_all ; \
	cd -

.PHONY: build
build:
	mkdir -p build
	cd build && \
	cmake .. && \
	make -j$(nproc) ; \
	cd -
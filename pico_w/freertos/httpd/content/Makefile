picow_fsdata.c: build/makefsdata.linux fs/* ${PICO_SDK_PATH}/lib/lwip
	build/makefsdata.linux -f:$@

build/makefsdata.linux:
	mkdir -p build
	cmake -B build
	make -j8 -C build

clean:
	rm -fr build
	rm *.c
all:
	tar -xpf ../rtems_waf.tar
	cp ../gdtoa/arith.h .
	./waf configure --rtems=$$RTEMS --rtems-bsp=i386/pc386
	./waf
	$$RTEMS/bin/rtems-run --rtems-bsps=pc-qemu  ./build/i386-rtems6-pc386/hello.exe | grep define > ../gdtoa/gd_qnan.h

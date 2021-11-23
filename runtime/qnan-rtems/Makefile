all:
	tar -xpf ../rtems_waf.tar
	cp ../gdtoa/arith.h .
	./waf configure --rtems=$$HOME/quick-start/rtems/6 --rtems-bsp=i386/pc386
	./waf
	$$HOME/quick-start/rtems/6/bin/rtems-run --rtems-bsps=pc-qemu  ./build/i386-rtems6-pc386/hello.exe | grep define > ../gdtoa/gd_qnan.h

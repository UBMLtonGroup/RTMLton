all:
	tar -xpf ../../../runtime/rtems_waf.tar
	./waf configure --rtems=/opt/rtems6 --rtems-bsp=i386/pc386
	./waf
	/opt/rtems6/bin/rtems-run --rtems-bsps=pc-qemu  ./build/i386-rtems6-pc386/hello.exe 

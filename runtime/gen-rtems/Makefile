all: build makec makesml makeh makesizes
	touch ../gen/gen-types.stamp ../gen/gen-sizes.stamp

.PHONY: build 
build:
	tar -xpf ../rtems_waf.tar
	./waf configure --rtems=$$RTEMS --rtems-bsp=i386/pc386

.PHONY: makec
makec:
	cp wscript.gentypes wscript
	./waf
	$$RTEMS/bin/rtems-run --rtems-bsps=pc-qemu  ./build/i386-rtems6-pc386/hello.exe | fgrep '/* C */' | sed 's,/\* C \*/,,g' > ../gen/c-types.h

.PHONY: makesml
makesml:
	cp wscript.gentypes wscript
	./waf
	$$RTEMS/bin/rtems-run --rtems-bsps=pc-qemu  ./build/i386-rtems6-pc386/hello.exe | fgrep '(* SML *)' | sed 's,(\* SML \*),,g' > ../gen/c-types.sml

.PHONY: makec
makeh:
	cp wscript.gentypes wscript
	./waf
	$$RTEMS/bin/rtems-run --rtems-bsps=pc-qemu  ./build/i386-rtems6-pc386/hello.exe | fgrep '/* H */' | sed 's,/\* H \*/,,g' > ../gen/ml-types.h

makesizes:
	cp wscript.gensizes wscript
	./waf
	$$RTEMS/bin/rtems-run --rtems-bsps=pc-qemu  ./build/i386-rtems6-pc386/hello.exe  | fgrep '/* gensizes */' | sed 's,/\* gensizes \*/,,g' > ../gen/sizes

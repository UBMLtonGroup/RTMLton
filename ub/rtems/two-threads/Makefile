# steps
#  1. build the sml file, save the C intermediates
#  2. rename 'main' in the *.0.c file
#  3. unpack the waf code
#  4. configure and build using waf

all: rtems

rtems: fixmain fixprotos
	rm -fr build
	tar -xpf ../../../runtime/rtems_waf.tar
	./waf configure --rtems=/opt/rtems6 --rtems-bsp=i386/pc386
	./waf
	/opt/rtems6/bin/rtems-run --rtems-bsps=pc-qemu  ./build/i386-rtems6-pc386/hello.exe 

buildsml:
	[ -f /home/jcmurphy/RTMLton/build/lib/targets/self/constants.linux ] || cp /home/jcmurphy/RTMLton/build/lib/targets/self/constants /home/jcmurphy/RTMLton/build/lib/targets/self/constants.linux
	cp /home/jcmurphy/RTMLton/build/lib/targets/i386-rtems6/constants /home/jcmurphy/RTMLton/build/lib/targets/self/constants
	/home/jcmurphy/RTMLton/build/bin/mlton -codegen c  -keep g -debug true two-threads.sml

buildlinux:
	/home/jcmurphy/RTMLton/build/bin/mlton -codegen c  -keep g -debug true two-threads.sml

undoconstants:
	cp /home/jcmurphy/RTMLton/build/lib/targets/self/constants.linux /home/jcmurphy/RTMLton/build/lib/targets/self/constants

fixmain: buildsml
	mv two-threads.0.c two-threads.0.c.orig
	sed 's/^int main /int mainX /' two-threads.0.c.orig | \
	sed 's/uint32_t atMLtons_len = 2;/uint32_t atMLtons_len = 6;/' | \
	sed 's/static char\* atMLtons/static char* atMLtons[] = {"@MLton","rtthreads", "true","max-heap", "4M", "--",}; static char* atMLtonsOLD/' \
	> two-threads.0.c

fixprotos: buildsml
	mv two-threads.3.c two-threads.3.c.orig
	cat two-threads.3.c.orig | \
	sed 's/PRIVATE Int32 Posix_FileSys_Stat_getCTime ();/PRIVATE C_Time_t Posix_FileSys_Stat_getCTime(void);/' | \
	sed 's/PRIVATE Int32 Posix_FileSys_Stat_getMTime ();/PRIVATE C_Time_t Posix_FileSys_Stat_getMTime(void);/' | \
	sed 's/PRIVATE Int32 Posix_FileSys_Stat_getATime ();/PRIVATE C_Time_t Posix_FileSys_Stat_getATime(void);/' | \
	sed 's/PRIVATE Word32 Posix_FileSys_Stat_getGId ();/PRIVATE C_GId_t Posix_FileSys_Stat_getGId(void);/' | \
	sed 's/PRIVATE Word32 Posix_FileSys_Stat_getUId ();/PRIVATE C_UId_t Posix_FileSys_Stat_getUId(void);/' | \
	sed 's/PRIVATE Word32 Posix_FileSys_Stat_getNLink ();/PRIVATE C_NLink_t Posix_FileSys_Stat_getNLink(void);/' \
	> two-threads.3.c 


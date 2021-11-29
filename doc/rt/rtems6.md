## RTEMS 6

Refer to [RTEMS doc](https://docs.rtems.org/branches/master/user/start/sources.html#git) for full details. 

### Checkout RTEMS

```
mkdir -p $HOME/quick-start/src
cd $HOME/quick-start/src
git clone git://git.rtems.org/rtems-source-builder.git rsb
git clone git://git.rtems.org/rtems.git
```

### Build RTEMS

```
cd $HOME/quick-start/src/rsb/rtems
../source-builder/sb-set-builder --prefix=$HOME/quick-start/rtems/6 6/rtems-i386
../source-builder/sb-set-builder --prefix=$HOME/quick-start/rtems/6 6/rtems-sparc
../source-builder/sb-set-builder --prefix=$HOME/quick-start/rtems/6 6/rtems-arm

jcmurphy@ubuntu32:~/quick-start/src/rsb/rtems$ ../source-builder/sb-set-builder --prefix=$HOME/quick-start/rtems/6     --with-rtems-tests=yes bsps/pc
```

### Configure for qemu x86 

```
apt install qemu
cat <<EOF > $HOME/quick-start/src/rtems/config.ini
[i386/pc386]
BUILD_TESTS = True
EOF
```

### Build RTMLton 

`./compile-rtems.sh`


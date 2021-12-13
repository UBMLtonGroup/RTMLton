## RTEMS 6

Refer to [RTEMS doc](https://docs.rtems.org/branches/master/user/start/sources.html#git) for full details. 

### Build RTMLton

`./compile.sh`

This will build the x86 runtime and compiler, and install it into `build`

We will need that for one manual step below (to create the `constants` file).

### Build and install RTEMS i386/pc386

After this finishes, add `/opt/rtems6/bin` to your path.


```
mkdir -p /opt/src /opt/rtems6


cd /opt/src && \
    git clone --depth 1 git://git.rtems.org/rtems-source-builder.git rsb && \
    cd /opt/src/rsb && \
    ./source-builder/sb-check && \
    cd /opt/src/rsb/rtems && \
    ../source-builder/sb-set-builder --prefix=/opt/rtems6 6/rtems-i386 && \
    cd /usr/opt && \
    rm -rf rsb

export PATH=/opt/rtems6/bin:${PATH}

cat > /tmp/config.ini <<EOF
[DEFAULT]
RTEMS_POSIX_API = True
BUILD_TESTS = True

[i386/pc386]
EOF

cd /opt/src && \
    git clone --depth 1 git://git.rtems.org/rtems.git rtems && \
    cd rtems && cp /tmp/config.ini . && \
    ./waf configure \
    --prefix=/opt/rtems6 && \
    ./waf && \
    ./waf install
```

### Build RTMLton for RTEMS

`./compile-rtems.sh`

This will leave the runtime (only) in `build/lib/targets/i386-rtems6`

We now need to build the `constants` file.

```
cd runtime/constants-rtems
RTEMS=/opt/rtems6 make
```

We see for example that `fl_lock_*` is at 1420 for a native build ("self") but is at a different location for the RTEMS build.

```
$ grep fl_lock build/lib/targets/*/constants
build/lib/targets/i386-rtems6/constants:fl_lock_Offset = 1012
build/lib/targets/i386-rtems6/constants:fl_lock_Size = 56
build/lib/targets/self/constants:fl_lock_Offset = 1420
build/lib/targets/self/constants:fl_lock_Size = 24
```


### Hello world app.

We deal with the above differences by swapping the `constants` file. This isn't ideal, but as of this
writing, the compiler itself is not aware of the RTEMS target platform, so we can't just say `-target i386-rtems`

```
$ cd ub

# this doesnt work

$ ../build/bin/mlton -target i386-rtems6 -keep g -verbose 1 justprint.sml
invalid target: i386-rtems6

# this works

$ ../build/bin/mlton -target self -verbose 1 -keep g justprint.sml

# save the native constants and install our rtems one

$ cp ../build/lib/targets/self/constants ../build/lib/targets/self/constants.native
$ cp ../build/lib/targets/i386-rtems6/constants ../build/lib/targets/self/constants

# notice the native offset..

$ grep Lock_fl justprint.?.c | head -1
justprint.1.c:	Lock_fl (O(CPointer, GCState, 1420));

$ ../build/bin/mlton -target self -verbose 1 -keep g justprint.sml

# notice now the offset is different

$ grep Lock_fl justprint.?.c | head -1
justprint.1.c:	Lock_fl (O(CPointer, GCState, 1012));


$ cd rtems/justprint
$ cp ../../justprint.?.c .

$ vi justprint.1.c
# rename "main" to "mainX" at the bottom

$ RTEMS=/opt/rtems6 make
[various output]
something
more stuff
^^^ these two lines are the hello world part. you should see them near the bottom.
```





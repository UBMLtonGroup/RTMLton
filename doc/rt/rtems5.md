## RTEMS 6

Refer to [RTEMS doc](https://docs.rtems.org/branches/master/user/start/sources.html#git) for full details. 


### Build and install RTEMS i386/pc386

After this finishes, add `/opt/rtems6/bin` to your path.


```
mkdir -p /opt/src /opt/rtems5


cd /opt/src && \
    git clone -b 5 --depth 1 git://git.rtems.org/rtems-source-builder.git rsb && \
    cd /opt/src/rsb && \
    ./source-builder/sb-check && \
    cd /opt/src/rsb/rtems && \
    ../source-builder/sb-set-builder --prefix=/opt/rtems5 5/rtems-i386 && \
    cd /usr/opt && \
    rm -rf rsb

export PATH=/opt/rtems5/bin:${PATH}

cat > /tmp/config.ini <<EOF
[DEFAULT]
RTEMS_POSIX_API = True
BUILD_TESTS = True

[i386/pc386]
EOF

cd /opt/src && \
    git clone -b 5 --depth 1 git://git.rtems.org/rtems.git rtems && \
    cd rtems && cp /tmp/config.ini . && \
    ./waf configure \
    --prefix=/opt/rtems5 && \
    ./waf && \
    ./waf install
    ```


### Build RTMLton 

`./compile-rtems.sh`


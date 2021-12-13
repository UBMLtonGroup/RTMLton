#!/bin/bash
#
# this builds rtems6 in /opt/rtems6
# the rtmlton compile-rtems.sh script expects it in this location

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

#!/bin/bash
# configuration for my GCC compiler (trunk 16)

/usr/src/Lang/gcc/configure --enable-languages=c,c++,jit \
    --program-suffix=-trunk --prefix=/usr/local/ \
    --with-gcc-major-version-only --enable-shared \
    --enable-linker-build-id --without-included-gettext \
    --enable-threads=posix --enable-nls --enable-bootstrap \
    --enable-clocale=gnu --enable-libstdcxx-debug \
    --enable-libstdcxx-time=yes --with-default-libstdcxx-abi=new \
    --enable-libstdcxx-backtrace --enable-gnu-unique-object \
    --disable-vtable-verify --enable-plugin --enable-default-pie \
    --with-system-zlib --enable-libphobos-checking=release \
    --with-target-system-zlib=auto --enable-objc-gc=auto \
    --disable-multiarch --disable-werror --enable-cet --disable-multilib \
    --with-tune=native --enable-offload-defaulted \
    --enable-gnu-unique-object --disable-vtable-verify --enable-plugin \
    --enable-default-pie --with-system-zlib --disable-werror --enable-cet \
    --with-tune=native --enable-offload-defaulted \
    --enable-checking=release --build=x86_64-linux-gnu \
    --host=x86_64-linux-gnu --target=x86_64-linux-gnu \
    --with-build-config=bootstrap-lto-lean --enable-link-serialization=3 \
    --enable-libgccjit --enable-host-shared --enable-libgdiagnostics

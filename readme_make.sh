#!/bin/bash

./configure --host=arm-anykav200-linux-uclibcgnueabi --prefix=$(pwd)/curl/ --without-nss --without-ssl \
    --enable-dynamic --enable-optimize --disable-debug --disable-curldebug --disable-symbol-hiding --disable-dict \
    --disable-gopher --disable-imap --disable-pop3 --disable-rtsp --disable-smtp --disable-telnet --disable-sspi \
    --disable-smb --disable-ntlm-wb --disable-tls-srp --disable-soname-bump --disable-manual --disable-file \
    --disable-ldap --disable-tftp --enable-http --disable-ftp --disable-ipv6


make -j4 && make install

#! /bin/sh

exec gdb $* </dev/tty >/dev/tty 2>/dev/tty

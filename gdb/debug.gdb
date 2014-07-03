#!/usr/bin/gdb -x

# Example of a live showcase of stages only interesting for the example

file memu.elf

# non-stop mode:
#   on  = only stop threads hitting breakpoint. You can use C-c c to stop any
#         running thread. Stepping or continuing execution acts on the currently
#         selected thread (see `info threads`).
#         Use `c -a` to continue execution of every stopped threads.
#
#   off (aka all-stop mode) =
#         stop every threads when hitting a breakpoint. Stepping or continuing
#         execution acts on every threads.
set non-stop on

# Showcase of Unit Signals Generation done by the ID Stage in the ID/EX latch.
b IDStage::operator()

# run this with this test file:
r test/t3

# run this script with gdb -x gdb/debug.gdb or with gdb command
# `(gdb) so gdb/debug.gdb`

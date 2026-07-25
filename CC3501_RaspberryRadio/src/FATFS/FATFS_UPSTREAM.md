# FatFs upstream

This directory vendors ChaN FatFs R0.16 (`FFCONF_DEF`/`FF_DEFINED` 80386),
including the upstream `ffunicode.c` module required by long-file-name mode.

The upstream copyright and redistribution terms are retained in `ff.c` and
`ff.h`. Project-specific options are in `ffconf.h`; long-file-name support is
enabled for the requested log directory/file names, and timestamps are supplied
by the application through `get_fattime()`.

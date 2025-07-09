switch-sys-tweak
================

[![Build](https://github.com/p-sam/switch-sys-tweak/workflows/Build/badge.svg?branch=master)](https://github.com/p-sam/switch-sys-tweak/actions?query=workflow%3ABuild)

A collection of miscellaneous mitms that may eventually grow as stuff gets added.

## Features

* `FEAT_NSAM_CONTROL` : [5.1.0+] Mitm's `ns:am2`->GetReadOnlyApplicationControlDataInterface to override icon/author/version/name by title.
* `FEAT_NSRO_CONTROL` : [11.0.0+] Same hook as above, but mitm target is `ns:ro`

## Toggles

* `TOGL_LOGGING` : Enable logging to "sdmc:/sys-tweak.log"

## How to compile

Add the required flags to your make command line. `FEAT_ALL` and `TOGL_ALL` are available as wildcard flags.

Example: `make FEAT_ALL="Y" TOGL_LOGGING="Y"`

**Note:** You should probably `make clean` if you change the flags you're using.

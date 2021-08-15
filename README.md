## coreboot boot configuration editor

This is a terminal application for changing boot settings in ROM files of
[coreboot][coreboot].  It can change boot order and values of
options in interactive or automated mode.  Requires `cbfstool` to read and
update ROM files.

### Dependencies

* `cbfstool`
* `libcurses`
* `libreadline`
* `GNU Make`

### Building

```bash
make # or `make debug` (possibly after `make clean`)
./sortbootorder -h
```

### Usage example

Non-interactively:

```bash
# path to cbfstool is specified explicitly
sortbootorder coreboot.rom -c build/cbfstool -b USB,SATA -o usben=off -o watchdog=300
```

Interactively:

```bash
# cbfstool is available in $PATH
sortbootorder coreboot.rom
```

### Controls in interactive mode

Navigation can be done with extended keys (arrows, etc.), CLI-like shortcuts or
in Vim-like manner.  CLI-like shortcuts are associated with items on the screen
and are upper case only, so hold `Shift` to perform navigation in that manner.
Bottom of each screen provides list of keys and their meaning.

### License

GPL 2.0 or later.

[coreboot]: https://www.coreboot.org/

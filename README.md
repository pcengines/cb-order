## coreboot boot configuration editor

This is a terminal application for changing boot settings in ROM files of
[coreboot][coreboot].  It can change boot order and values of
options in interactive or automated mode.

### Dependencies

* `libcurses`
* `GNU Make`

### Building

```bash
make # or `make debug` (possibly after `make clean`)
./cb-order -h
```

### Usage example

Non-interactively:

```bash
cb-order coreboot.rom -b USB,SATA -o usben=off -o watchdog=300
```

Interactively:

```bash
cb-order coreboot.rom
```

### Controls in interactive mode

Navigation can be done with extended keys (arrows, etc.), CLI-like shortcuts or
in Vim-like manner.  CLI-like shortcuts are associated with items on the screen
and are upper case only, so hold `Shift` to perform navigation in that manner.
Bottom of each screen provides list of keys and their meaning.

### License

GPL 2.0 only.

[coreboot]: https://www.coreboot.org/

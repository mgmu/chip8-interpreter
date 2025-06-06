# CHIP8 interpreter
An interpreter for the CHIP-8 programming language written in C.

## How to run
Compile program with:

`
make
`

Then run the `chip8` executable with at least two arguments:

`
./chip8 <filename> <scale factor> [<debug mode>]
`

The `<debug mode>` argument is optional. When a third argument is provided, the
interpreter runs in debug mode that prints to standard output the state of the
registers and the instruction code to run, at the begining of each processor
cycle.

This repository provides a `roms` directory with CHIP-8 programs, see its README
to get a list of the ones that can be executed with this interpreter.

> **_Note:_** If you want to run other ROMs, make sure they contain only the
program part and not another interpreter, as this interpreter loads the ROM file
content at address `0x0200` in RAM.

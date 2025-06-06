# CHIP-8 interpreter
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

`<filename>` is a CHIP-8 program file. `<scale factor>` is a strictly positive
integer (>0). If 1 is given, the display maintains its original size (64 x 32),
on my computer a scale factor of 20 is good.

The `<debug mode>` argument is optional. When a third argument is provided, the
interpreter runs in debug mode that prints to standard output the state of the
registers and the instruction code to run, at the begining of each processor
cycle.

This repository provides a `roms` directory with CHIP-8 programs, see its README
to get a list of the ones that can be executed with this interpreter.

> **_Note:_** If you want to run other ROMs, make sure they contain only the
program part and not another interpreter, as this interpreter loads the ROM file
content at address `0x0200` in RAM.

### Processor error codes
If an error occurs, the processor stops execution and the interpreter quits. The
error code is output to standard output. Here is a table of the currently
detected errors and their codes:

| Error | Code | Description |
|-------|------|-------------|
| OUT_OF_RAM_ERR | 1 | The instruction to run is at an address beyond the RAM |
| EXEC_ERR | 2 | The decoder and executer indicated in error |

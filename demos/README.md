Demos testing:

- `test_opcode.ch8`: Works.

- `IBM Logo.ch8`: Works.

- `Chip8 Picture.ch8`: Works.

- `Delay Timer Test [Matthew Mikolay, 2010].ch8`: Works, but proc runs too fast.

- `Keypad Test [Hap, 2006].ch8`: Works, every character sprite is lit up when
corresponding key is pressed (DOWN then UP).

- `Fishie [Hap, 2005].ch8`: Works, the fish image is displayed.

- `15 Puzzle [Roger Ivie] (alt).ch8`: A press to 'D' ('R' on my keyboard)
refreshes game. The other keys do not seem to work.

- `Astro Dodge [Revival Studios, 2008].ch8`: Program quits on error indicating
`File is corrupted`. The file is indeed 1113 bytes long, which is not a sequence
of 2 bytes long instructions.

- `Clock Program [Bill Fisher, 1981].ch8`: Program blocks on instruction 0xF10A,
which corresponds to "Wait for a key press, store the value of the key in Vx.".
My implementation uses a while loop that ends when a key is pressed, but this
can not work as the program cannot therefore check for key presses. Solution:
remove the while loop and decrement `pc` by 2 to rerun the check *after* the key
events have been handled, a flag must be declared too, and set to 1 when a key
was indeed pressed (in order to not repeat the instruction event if a key was
pressed). Works, but as 60Hz frequency is not respected the clock goes too fast.
A note on clock configuration, the user must be really quick to not trigger
multiple events, so clock configuration is hard. Update on clock configuration,
after adding check of previous keyboard state, the clock configuration is much
easier.

- `Random Number Test [Matthew Mikolay, 2010].ch8`: When a key is pressed, a
random number should be generated, but instead the key pressed is displayed.
That is because of a typing error, where that instruction performed an equality
check (`==`) instead of an assignment (`=`)... Now it works.

- `Framed MK1 [GV Samways, 1980].ch8` and `Framed MK2 [GV Samways, 1980].ch8`:
Now that randomizer works, it works.

- `Life [GV Samways, 1980].ch8`: Keypad seems to work fine, albeit cpu works too
fast. The display seems odd, so I don't think it works properly.
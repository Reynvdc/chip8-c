#Chip 8 emulator in C

##instructions to run the emulator

On Mac:
install SDL2 with homebrew
`brew install sdl2`

Afterwards you can run the program with

`clang chip8.c -I/opt/homebrew/include -L/opt/homebrew/lib -lSDL2 -o chip8 && ./chip8`

# Movie Hangman

A Hangman-style guessing game for the [Zeal 8-bit Computer](https://zeal8bit.com/), built on
[Zeal 8-bit OS](https://github.com/Zeal8bit/Zeal-8-bit-OS) and the
[Zeal Video Board SDK](https://github.com/Zeal8bit/Zeal-VideoBoard-SDK). Players guess the
title of a well-known movie one letter at a time using the on-screen A-Z keyboard, with a
classic hangman drawing that fills in with each wrong guess.

## Features

- Tile-based graphics rendered through `zvb_gfx`, including background, letter-clue, and
  text tilemap layers exported from Tiled (`map/`).
- Custom tileset and sprite art authored in Aseprite/GIMP (`img/`).
- A generated word list of well-known movies from the last 25 years (`src/movies.h`/`.c`),
  produced by `tools/fetch_movies.py` from TMDb data.

## Project layout

```
autoexec.zs      Zeal OS autoexec script used when booting the game
Makefile         Build rules (SDCC/SDAS/SDLD toolchain)
img/             Source art assets and exported Zeal tileset/tilemap data
map/             Tiled map project and exported Zeal map data
src/             Game source (C and Z80 asm)
tools/           Helper scripts (e.g. movie list generator)
obj/, bin/       Build output (generated)
```

## Building

Requires the [SDCC](http://sdcc.sourceforge.net/) toolchain and local checkouts of
`Zeal-8-bit-OS` and `Zeal-VideoBoard-SDK` (see `ZOS_PATH` / `ZVB_SDK_PATH` in the
[Makefile](Makefile)).

```sh
make
```

This produces `bin/cylix.bin`, which is copied to the `s` disk image folder for use with the
Zeal 8-bit Computer or its emulator.

## Generating the movie word list

`tools/fetch_movies.py` pulls the top-grossing movies per year from
[TMDb](https://www.themoviedb.org/) and writes `src/movies.h`/`src/movies.c`. It requires a
free TMDb API key:

```sh
python3 tools/fetch_movies.py --api-key YOUR_KEY --start-year 2001 --end-year 2025 --count 10
```

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) for details.

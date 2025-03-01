# cp1-parser

The program accepts a file written in a programming language described [here](https://gist.github.com/TurtlePU/0f74a0ad7783704e28bc08bc6bb95acb), and then performs:
 1. Tokenization (splits the initial file in tokens)
 2. Parsing (using a sequence of tokens, creates a machine-readable structure called AST)
 3. Formatting (outputs beautified file from AST)

## Requirements
Dependencies required for building the executable are:
 - C++ compiler (workability was checked using `clang++`, though any other relatively new and C++20 supporting should work too)
 - `cmake`, `make` to build the executable

## Build
Building is done as usual:
```bash
$ mkdir build
$ cd build/
$ cmake ..
$ make
$ ./beautify from to  # example of usage
```

## Usage
Call `beautify` executable to format `read_from` and output to `write_to`:

`$ ./beautify read_from [write_to]`

For more, call `beautify` executable for help:

`$ ./beautify`

## Roadmap
 - [x] Tokenization;
 - [x] Parsing into separate structures and classes;
 - [x] Beautify output;
 - [ ] Prod-ready to some degree.

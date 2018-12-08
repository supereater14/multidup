# multidup

## A one-to-many file and disk duplication tool

Multidup is a file and disk duplication tool. It operates in a similar fashion
to dd, reading blocks from one file and writing them to another.

Multidup is particularly designed for large file and disk copying.

### Comparison to dd

Unlike dd, multidup is designed to copy one source to multiple destinations.
When copying to multiple destinations, copy operations are conducted in parallel
and progress is displayed individually for each operation.

Multidup is much less configurable than dd, so it may not be an option in more
complex operations.

## Usage

Multidup takes an arbitrary number of arguments. The first argument is taken as
the source, and subsequent arguments are destinations.

```
multidup [input file] [output file] {[output file]...}
```

While transferring, an interface will be displayed showing the progress of each
copy operation. This interface updates every 5 seconds.

## Installation

### Requirements

Multidup is designed to run under a POSIX-compliant environment. The main
requirements are gcc, pthreads and a UNIX-like kernel interface.

## Building

After cloning the repository, simply run

```
make clean
make
```

After building, it cen be installed with

```
sudo make install
```

Once installed, multidup can be removed with

```
sudo make remove
```

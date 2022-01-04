# Selective-PMO

H. Kojima et al., “Improved Probability Modeling for Lossless Image Coding Using Example Search and Adaptive Prediction”, IWAIT2022.

## Installation

1. Install the following requirements

    - CMake (tested with 3.17.0)
    - GCC (tested with 9.3.0) or Intel C++ Compiler (tested with 2021.2.0)

2. Generate a build file using CMake.

```bash
$ cd bin
$ cmake ..
```

3. Compile the project.

```bash
$ make
```

## Usage

0. Print the usage of the options

```bash
$ ./pmo --help
```

1. Encode (.pgm to .pmo)

```bash
$ ./pmo -i /path/to/input/image.pgm -b /path/to/binary/file.pmo
```

2. Decode (.pmo to .pgm)

```bash
$ ./pmo -b /path/to/binary/file.pmo -o /path/to/output/image.pgm
```

3. Encode and Decode ( Check distortion-free )

```bash
$ ./pmo -i /path/to/input/image.pgm -b /path/to/binary/file.pmo -o /path/to/output/image.pgm
```

## Authors

Hiroki KOJIMA (Maintainer), Diego FUJII

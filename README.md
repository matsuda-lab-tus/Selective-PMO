# Selective-PMO

H. Kojima et al., “Improved Probability Modeling for Lossless Image Coding Using Example Search and Adaptive Prediction”, IWAIT2022.

This implementation only supports 8-bpp grayscale images, raw PGM format.

## Coding rates (bits per pixel)

Selective-PMO achieves **the lowest coding rate (bpp)** for most test images compared to state-of-the-art methods.

| Image       | Selective-PMO | PMO-GMM | MRP   | Vanilc WLS D | TMW   | Original |
| ----------- |:-------------:|:-------:|:-----:|:------------:|:-----:|:--------:|
| Camera      | 3.804         | 3.833   | 3.949 | 3.995        | 4.098 | 8        |
| Couple      | 3.269         | 3.281   | 3.388 | 3.459        | 3.446 | 8        |
| Noisesquare | 5.274         | 5.296   | 5.270 | 5.159        | 5.542 | 8        |
| Airplane    | 3.529         | 3.546   | 3.591 | 3.575        | 3.601 | 8        |
| Baboon      | 5.611         | 5.698   | 5.663 | 5.678        | 5.738 | 8        |
| Lena        | 4.215         | 4.237   | 4.280 | 4.246        | 4.300 | 8        |
| Lennagrey   | 3.825         | 3.845   | 3.889 | 3.856        | 3.908 | 8        |
| Peppers     | 4.161         | 4.176   | 4.199 | 4.187        | 4.251 | 8        |
| Shapes      | 0.490         | 0.497   | 0.685 | 1.302        | 0.740 | 8        |
| Balloon     | 2.573         | 2.584   | 2.579 | 2.626        | 2.649 | 8        |
| Barb        | 3.708         | 3.733   | 3.815 | 3.815        | 4.084 | 8        |
| Barb2       | 4.122         | 4.146   | 4.216 | 4.231        | 4.378 | 8        |
| Goldhill    | 4.171         | 4.191   | 4.207 | 4.229        | 4.266 | 8        |
| *Average*   | *3.750*       | *3.774* |*3.826*|*3.874*       |*3.923*| *8*      |

#### PMO-GMM

Lossless Image Coding Exploiting Local and Non-local Information via Probability Model Optimization [[paper](https://ieeexplore.ieee.org/abstract/document/8903128)]

#### MRP

Lossless coding using variable block-size adaptive prediction optimized for each image [[paper](https://ieeexplore.ieee.org/document/7078076)] [[source](https://www.rs.tus.ac.jp/matsuda-lab/matsuda/mrp/index.html)]

#### Vanilc WLS D

Probability Distribution Estimation for Autoregressive Pixel-Predictive Image Coding [[paper](https://ieeexplore.ieee.org/abstract/document/7393808)] [[source](https://github.com/siemens/vanilc)]

#### TMW

TMW - a New Method for Lossless Image Compression [[paper](http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.116.3891)] [[source](https://web.archive.org/web/20050914221204/http://www.csse.monash.edu.au/~bmeyer/tmw/)]

#### Original

All test images are from [here](https://web.archive.org/web/20050914221204/http://www.csse.monash.edu.au/~bmeyer/tmw/).

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

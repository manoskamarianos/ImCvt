 ![language](https://img.shields.io/badge/language-C-green.svg) ![build](https://img.shields.io/badge/build-Windows-blue.svg) ![build](https://img.shields.io/badge/build-linux-FF1010.svg) 

ImCvt
===========================

**ImCvt** is a **image format converter** which supports several image formats. **ImCvt** is written in C (C99), contains the simplest, standalone implementation of the encoders/decoders of some image formats, the purpose is to help one understand these image compression formats and algorithms swiftly.

**ImCvt** supports:

|                            format                            | suffix |                       supported color                        | convert from (decode)                     | convert to (encode)                       |               source code               |
| :----------------------------------------------------------: | :----: | :----------------------------------------------------------: | :---------------------------------------- | :---------------------------------------- | :-------------------------------------: |
|    **[PNM](https://netpbm.sourceforge.net/doc/pnm.html)**    |  .pnm  | ![gray8](https://img.shields.io/badge/-Gray8-lightgray.svg) ![rgb24](https://img.shields.io/badge/-RGB24-c534e0.svg) | :white_check_mark: fully support          | :white_check_mark: fully support          |  [150 lines of C](./src/imageio_pnm.c)  |
|         **[PNG](https://en.wikipedia.org/wiki/PNG)**         |  .png  | ![gray8](https://img.shields.io/badge/-Gray8-lightgray.svg) ![rgb24](https://img.shields.io/badge/-RGB24-c534e0.svg) | :negative_squared_cross_mark: unsupported | :ballot_box_with_check: partially support |  [240 lines of C](./src/imageio_png.c)  |
|   **[BMP](https://en.wikipedia.org/wiki/BMP_file_format)**   |  .bmp  | ![gray8](https://img.shields.io/badge/-Gray8-lightgray.svg) ![rgb24](https://img.shields.io/badge/-RGB24-c534e0.svg) | :ballot_box_with_check: partially support | :ballot_box_with_check: partially support |  [180 lines of C](./src/imageio_bmp.c)  |
|              **[QOI](https://qoiformat.org/)**               |  .qoi  |   ![rgb24](https://img.shields.io/badge/-RGB24-c534e0.svg)   | :white_check_mark: fully support          | :white_check_mark: fully support          |  [240 lines of C](./src/imageio_qoi.c)  |
|     **[JPEG-LS](https://www.itu.int/rec/T-REC-T.87/en)**     |  .jls  | ![gray8](https://img.shields.io/badge/-Gray8-lightgray.svg) ![rgb24](https://img.shields.io/badge/-RGB24-c534e0.svg) | :negative_squared_cross_mark: unsupported | :ballot_box_with_check: partially support |  [480 lines of C](./src/imageio_jls.c)  |
|  **[JPEG-LS ext](https://www.itu.int/rec/T-REC-T.870/en)**   | .jlsx  | ![gray8](https://img.shields.io/badge/-Gray8-lightgray.svg)  | :ballot_box_with_check: support           | :ballot_box_with_check: support           | [760 lines of C](./src/imageio_jlsx.c)  |
| **[H.265](https://en.wikipedia.org/wiki/High_Efficiency_Video_Coding)** | .h265  | ![gray8](https://img.shields.io/badge/-Gray8-lightgray.svg)  | :negative_squared_cross_mark: unsupported | :ballot_box_with_check: partially support | [1730 lines of C](./src/imageio_hevc.c) |

Illustration of these image formats:

|                            format                            | Illustration                                                 |
| :----------------------------------------------------------: | :----------------------------------------------------------- |
|    **[PNM](https://netpbm.sourceforge.net/doc/pnm.html)**    | **[PNM](https://netpbm.sourceforge.net/doc/pnm.html)** (Portable Any Map), **[PGM](https://netpbm.sourceforge.net/doc/pgm.html)** (Portable Gray Map), and **[PPM](https://netpbm.sourceforge.net/doc/ppm.html)** (Portable Pix Map) are simple uncompressed image formats which store raw pixels. |
|         **[PNG](https://en.wikipedia.org/wiki/PNG)**         | **[PNG](https://en.wikipedia.org/wiki/PNG)** (Portable Network Graph) is the most popular lossless image compression format. |
|   **[BMP](https://en.wikipedia.org/wiki/BMP_file_format)**   | **[BMP](https://en.wikipedia.org/wiki/BMP_file_format)** (Bitmap Image File) is a popular uncompressed image formats which store raw pixels. |
|              **[QOI](https://qoiformat.org/)**               | **[QOI](https://qoiformat.org/)** (Quite OK Image) is a simple, fast lossless RGB image compression format. This project implements a simple QOI encoder/decoder in only 240 lines of C. |
|     **[JPEG-LS](https://www.itu.int/rec/T-REC-T.87/en)**     | **[JPEG-LS](https://www.itu.int/rec/T-REC-T.87/en)** is a lossless/lossy image compression standard which can get better grayscale compression ratio compared to PNG and Lossless-WEBP. JPEG-LS uses the maximum difference between the pixels before and after compression (NEAR value) to control distortion, **NEAR=0** is the lossless mode; **NEAR>0** is the lossy mode. The specification of JPEG-LS is [ITU-T T.87](https://www.itu.int/rec/T-REC-T.87/en) . **ImCvt** implements a simple JPEG-LS encoder in only 480 lines of C. However, **ImCvt** do not support JPEG-LS decoder yet. Here are two ways to decompress a .jls file: (1) Use [this website](https://products.groupdocs.app/viewer/JLS) to view .jls image online (but the website may not work sometimes). (2) Use [UBC's JPEG-LS encoder/decoder](./JPEG-LS_UBC_v2.2.zip) . |
| **[JPEG-LS extension](https://www.itu.int/rec/T-REC-T.870/en)** | This code is my implementation of **JPEG-LS extension** according to [ITU-T T.870](https://www.itu.int/rec/T-REC-T.870/en) specification for educational purpose. Note that the file header does NOT follow the specification! |
| **[H.265](https://en.wikipedia.org/wiki/High_Efficiency_Video_Coding)** | **[H.265/HEVC](https://en.wikipedia.org/wiki/High_Efficiency_Video_Coding)** is a video coding standard. This project implements a lightweight grayscale 8-bit H.265/HEVC intra-frame image compressor in only 1730 lines of C for educational purpose. Note that this code only offers H.265/HEVC compressor instead of decompressor. To decompress and view the compressed .h265 image file, you can use [File Viewer Plus](https://fileinfo.com/software/windows_file_viewer) or [Elecard HEVC Analyzer](https://elecard-hevc-analyzer.software.informer.com/) . For more knowledge about H.265, see [H.265/HEVC 帧内编码详解：CU层次结构、预测、变换、量化、编码](https://zhuanlan.zhihu.com/p/607679114) . |

More illustration about this [H.265 image encoder](./src/imageio_hevc.c):

- Quality parameter can be 0~4, corresponds to Quantize Parameter (QP) = 4, 10, 16, 22, 28.
- CTU : 32x32
- CU : 32x32, 16x16, 8x8
- TU : 32x32, 16x16, 8x8, 4x4
- The maximum depth of CU splitting into TUs is 1 (each CU is treated as a TU, or divided into 4 small TUs, while the small TUs are not divided into smaller TUs).
- part_mode: The 8x8 CU is treated as a PU (`PART_2Nx2N`), or divided into 4 PUs (`PART_NxN`)
- Supports all 35 prediction modes
- Simplified RDOQ (Rate Distortion Optimized Quantize)

　

　

# Compile

### compile in Windows (MinGW)

If you installed MinGW, run following compiling command in CMD:

```powershell
gcc src\*.c -O3 -o ImCvt.exe
```

which will get executable file [**ImCvt.exe**](./ImCvt.exe)

### compile in Windows (MSVC)

Also, you can use MSVC to compile. If you added MSVC (cl.exe) to your environment, run following compiling command in CMD:

```powershell
cl src\*.c /Ox /FeImCvt.exe
```

which will get executable file [**ImCvt.exe**](./ImCvt.exe)

### compile in Linux (gcc)

```bash
gcc src/*.c -O3 -o ImCvt
```

which will get Linux binary file **ImCvt**

　

　

# Usage

Run the program without any parameters to display usage:

```
> .\ImCvt.exe
|-----------------------------------------------------------------------|
| ImageConverter (v0.6)  by https://github.com/WangXuan95/              |
|-----------------------------------------------------------------------|
| Usage:                                                                |
|   ImCvt [-switches]  <in1> -o <out1>  [<in2> -o <out2>]  ...          |
|                                                                       |
|-----------------------------------------------------------------------|
| Where <in> and <out> can be:                                          |
|--------|---------------------------------|-------|--------|-----|-----|
|        |                                 | gray  | RGB    |     |     |
| suffix |          format name            | 8-bit | 24-bit | in  | out |
|--------|---------------------------------|-------|--------|-----|-----|
| .bmp   | Bitmap Image File (BMP)         | yes   | yes    | yes | yes |
| .pnm   | Portable Any Map (PNM)          | yes   | yes    | yes | yes |
| .pgm   | Portable Gray Map (PGM)         | yes   | no     | yes | yes |
| .ppm   | Portable Pix Map (PPM)          | no    | yes    | yes | yes |
| .qoi   | Quite OK Image (QOI)            | no    | yes    | yes | yes |
| .png   | Portable Network Graphics (PNG) | yes   | yes    | no  | yes |
| .jls   | JPEG-LS Image (JLS)             | yes   | yes    | no  | yes |
| .jlsx  | JPEG-LS extension (ITU-T T.870) | yes   | no     | yes | yes |
| .h265  | H.265/HEVC Image                | yes   | no     | no  | yes |
|--------|---------------------------------|-------|--------|-----|-----|
|                                                                       |
| switches:    -f         : force overwrite of output file              |
|              -<number>  : 1. PNG RGB palette quantize color count     |
|                           2. JPEG-LS near value                       |
|                           3. H.265 (qp-4)/6 value                     |
|-----------------------------------------------------------------------|
```

### Example Usage

convert a PNM to a QOI:

```powershell
ImCvt.exe image\P6.pnm -o image\P6.qoi
```

convert a QOI to a BMP:

```powershell
ImCvt.exe image\P6.qoi -o image\P6.bmp
```

convert a BMP to a JPEG-LS, with near=1 (lossy):

```powershell
ImCvt.exe image\P6.bmp -o image\P6.jls -1
```

convert a BMP to a H.265 image file, with (qp-4)/6=2 (lossy):

```powershell
ImCvt.exe image\P6.bmp -o image\P6.h265
```

convert a BMP to a PNG:

```powershell
ImCvt.exe image\P6.bmp -o image\P6.png
```

convert a BMP to a PNG by quantize to 16-color palette:

```powershell
ImCvt.exe image\P6.bmp -o image\P6_16.png -16
```

convert a grayscale PNM to JPEG-LS extension with near=1 (lossy), and then convert it back to BMP:

```
ImCvt.exe image\P5.pnm  -o image\P5.jlsx -1
ImCvt.exe image\P5.jlsx -o image\P5.bmp
```

　

　

# Related links

[1] Official website of QOI : https://qoiformat.org/

[2] ITU-T T.87 : JPEG-LS baseline specification : https://www.itu.int/rec/T-REC-T.87/en

[3] UBC's JPEG-LS baseline Public Domain Code : http://www.stat.columbia.edu/~jakulin/jpeg-ls/mirror.htm

[4] pillow-jpls library for Python :  https://pypi.org/project/pillow-jpls

[5] PNM Image File Specification : https://netpbm.sourceforge.net/doc/pnm.html

[6] uPNG: a simple PNG decoder: https://github.com/elanthis/upng

[7] FPGA-based Verilog QOI compressor/decompressor: https://github.com/WangXuan95/FPGA-QOI

[8] FPGA-based Verilog JPEG-LS encoder (basic version which support 8-bit gray lossless and lossy compression) : https://github.com/WangXuan95/FPGA-JPEG-LS-encoder

[9] FPGA-based Verilog JPEG-LS encoder (ultra high performance version which support 8-bit gray lossless compression) : https://github.com/WangXuan95/UH-JLS

[10] H.265/HEVC 帧内编码详解：CU层次结构、预测、变换、量化、编码：https://zhuanlan.zhihu.com/p/607679114

[11] The H.265/HEVC reference software HM: https://github.com/listenlink/HM

[12] NBLI: a fast, better lossless image format: https://github.com/WangXuan95/NBLI

[13] ITU-T T.870 : JPEG-LS extention specification : https://www.itu.int/rec/T-REC-T.870/en

[14] A comparison of many lossless image compression formats: https://github.com/WangXuan95/Image-Compression-Benchmark

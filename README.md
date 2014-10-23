# file-crawler
This simple app will crawl through a directory structure, find all of the files and keep a running count of the number of unique words it finds. It only reads real files and will ignore symlinks and special devices.

## Building
This was built and tested on Ubuntu 14.04 but should work on most linux distros. Install the standard build toolchain and the Boost libraries, clone this source, and then run make.

## Running
The binary requires a single positional option, the path to crawl over, and accepts an optional argument to specify the number of threads to use. If you have fast enough storage (SSD) you can increase the number of threads and watch the crawler speed up.

```
Usage: ssfi PATH [options]
Index all text files in PATH

Options:
  -h [ --help ]             show this help message
  -t [ --threads ] arg (=3) the number of file processor threads to use
```

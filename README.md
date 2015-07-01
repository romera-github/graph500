This code contains a subset of Duane Merrill's BC40 repository
of GPU-related functions, including his BFS implementation used
in the paper, Scalable Graph Traversals.

All copyrights reserved to their original owners.


###Goggle c++ Test Framework

To install the package in ~/usr/gtest/ as shared libraries, together with sample build as well:

    $ mkdir ~/temp
    $ cd ~/temp
    $ wget http://googletest.googlecode.com/files/gtest-1.7.0.zip
    $ unzip gtest-1.7.0.zip 
    $ cd gtest-1.7.0
    $ mkdir mybuild
    $ cd mybuild
    $ cmake -DBUILD_SHARED_LIBS=ON -Dgtest_build_samples=ON -G"Unix Makefiles" ..
    $ make
    $ cp -r ../include/gtest ~/usr/gtest/include/
    $ cp lib*.so ~/usr/gtest/lib


To validate the installation, use the following test.cpp as a simple test example:

    	#include <gtest/gtest.h>
    	TEST(MathTest, TwoPlusTwoEqualsFour) {
    		EXPECT_EQ(2 + 2, 4);
    	}
    	
    	int main(int argc, char **argv) {
    		::testing::InitGoogleTest( &argc, argv );
    		return RUN_ALL_TESTS();
    	}
    
To compile the test:

        $ export GTEST_HOME=~/usr/gtest
        $ export LD_LIBRARY_PATH=$GTEST_HOME/lib:$LD_LIBRARY_PATH
        $ g++ -I test.cpp $GTEST_HOME/include -L $GTEST_HOME/lib -lgtest -lgtest_main -lpthread 


###SIMDCompressionAndIntersection Library


As the name suggests, this is a C/C++ library for fast
compression and intersection of lists of sorted integers using
SIMD instructions. The library focuses on innovative techniques
and very fast schemes, with particular attention to differential
coding. It introduces new SIMD intersections schemes such as
SIMD Galloping.

This library can decode at least 4 billions of compressed integers per second on most
desktop or laptop processors. That is, it can decompress data at a rate of 15 GB/s.
This is significantly faster than generic codecs like gzip, LZO, Snappy or LZ4.

Authors: Leonid Boystov, Nathan Kurz and Daniel Lemire
with some contributions from Owen Kaser, Andrew Consroe and others.

[Documentation][#]


* Daniel Lemire, Leonid Boytsov, Nathan Kurz, SIMD Compression and the Intersection of Sorted Integers, Software Practice & Experience (to appear) http://arxiv.org/abs/1401.6399
* Daniel Lemire and Leonid Boytsov, Decoding billions of integers per second through vectorization, Software Practice & Experience 45 (1), 2015.  http://arxiv.org/abs/1209.2137 http://onlinelibrary.wiley.com/doi/10.1002/spe.2203/abstract
* Jeff Plaisance, Nathan Kurz, Daniel Lemire, Vectorized VByte Decoding, International Symposium on Web Algorithms 2015, 2015. http://arxiv.org/abs/1503.07387
* Wayne Xin Zhao, Xudong Zhang, Daniel Lemire, Dongdong Shan, Jian-Yun Nie, Hongfei Yan, Ji-Rong Wen, A General SIMD-based Approach to Accelerating Compression Algorithms, ACM Transactions on Information Systems 33 (3), 2015. http://arxiv.org/abs/1502.01916

[Simple demo][#]

Check out example.cpp

You can run it like so:

make example
./example

[Usage][#]

make
./unit

To run tests, you can do 

./testcodecs

(follow the instructions)


[For a simple C library][]


This library is a C++ research library. For something simpler,
written in C, see:

https://github.com/lemire/simdcomp


[Comparison with the FastPFOR C++ library][]


The FastPFOR C++ Library available at https://github.com/lemire/FastPFor
implements some of the same compression schemes except that
it is not optimized for the compression of sorted lists of integers.



[Licensing][]

Apache License, Version 2.0

As far as the authors know, this work is patent-free.

[Requirements][]

A CPU (AMD or Intel) with support for SSE2 (Pentium 4 or better) is required
while a CPU with SSE 4.1* (Penryn  [2007] processors or better) is recommended. 


A recent GCC (4.7 or better), Clang or Intel compiler.

A processor support AVX (Intel or AMD).

Tested on Linux and MacOS. It should be portable to Windows and other platforms.

*- The default makefile might assume AVX support, but AVX is not required. For GCC
compilers you might need the -msse2 flag, but you will not need the -mavx flag.

For advanced benchmarking, please see

advancedbenchmarking/README.md

where there is additional information as well as links to real data sets.



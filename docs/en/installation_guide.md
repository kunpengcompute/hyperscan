# Installation Guide<a name="EN-US_TOPIC_0000002520718454"></a>

## Overview<a name="EN-US_TOPIC_0000002550305593"></a>

This document describes how to install and compile Hyperscan based on the Kunpeng 920 processor and openEuler.

Hyperscan is a high-performance regular expression matching library. It is developed based on Perl Compatible Regular Expressions (PCRE) and is open-source under the Berkeley Software Distribution (BSD) license. It follows the regular expression syntax of the libpcre library but has its own C interfaces. Based on the official Hyperscan release and Kunpeng microarchitecture, the implementation mechanism of core interfaces is redesigned, the development and performance are optimized, and the software package suitable for the Kunpeng platform is released. Users of the Kunpeng platform can download this software package based on their service requirements to improve the stability and performance of services on the Kunpeng platform.

The following functions are added to the Hyperscan version dedicated for the Kunpeng platform:

- The Kunpeng platform branch fully compatible with Armv8-A is added. In addition, its use on the x86 platform is not affected.
- Neon instructions, inline assembly, data alignment, instruction alignment, memory prefetch, static branch prediction, and code restructuring are used to improve the performance on the Kunpeng platform.
- The Kunpeng Hyperscan Enhanced Library (KHSEL) is released. It includes a hybrid model with a short-rule bypass and a false-positive blocking model.
    - KHSEL optimizes the large-pattern matching algorithm FDR, small-pattern quick matching algorithm Shufti, and long-rule verification, and enhances the scan performance of Hyperscan for processing datasets such as snort_literal and snort_pcre.
    - The hybrid model with a short-rule bypass significantly improves the matching performance for rule sets containing short rules.
    - The false-positive blocking model greatly improves the matching performance for rule sets containing bad string fragments. Bad strings refer to a small number of rules with special fragments, causing excessive false positives in multi-pattern matching. This triggers a large number of interpreter calls and inefficient long-rule verification, but yields zero true matches. These unnecessary interpreter calls become computing hotspots, undermining the pre-filtering capability of multi-pattern matching.

For more information about Hyperscan, visit the [Kunpeng repository on GitCode](https://gitcode.com/boostkit/hyperscan).

## Environment Requirements<a name="EN-US_TOPIC_0000002550305599"></a>

### Verified Environments<a name="EN-US_TOPIC_0000002550345615"></a>

Hyperscan can function properly on Kunpeng 920 series processors running openEuler 22.03/openEuler 24.03 LTS SP4. If you encounter any problem with Hyperscan, confirm that your environment is a verified environment.

### Software Requirements<a name="EN-US_TOPIC_0000002550345611"></a>

Before installing and compiling software, obtain the software packages by referring to the links provided in this section.

[**Table 1**](#software-requirements) describes the software requirements.

**Table 1** Software requirements<a id="software-requirements"></a>

|Software Name|Version|Description|How to Obtain|
|--|--|--|--|
|GCC|10.3 or later|Required.|-|
|CMake|2.8.11 or later|Required.|-|
|Ragel|6.9 or later|Required. The compilation depends on Ragel.|[Link](http://www.colm.net/files/ragel/ragel-6.10.tar.gz)|
|Boost|1.57 or later|Required. The compilation depends on the Boost header file.|[Link](https://archives.boost.io/release/1.87.0/source/boost_1_87_0.tar.gz)|
|PCRE|8.41 or later|Optional. The compilation of the Hyperscan verification tool hscollider depends on PCRE 8.41 or later.|[Link](https://sourceforge.net/projects/pcre/files/pcre/8.43/pcre-8.43.tar.gz)|
|SQLite|SQLite 3|Optional. The compilation of the Hyperscan test tool hsbench depends on SQLite 3.|Use Yum to install it.|
|Hyperscan|master|Required. Software to be compiled.|[Link](https://gitcode.com/boostkit/hyperscan/tree/master)|

## Configuring the Compilation Environment<a name="EN-US_TOPIC_0000002550305597"></a>

### (Optional) Configuring the Local Repository<a name="EN-US_TOPIC_0000002550305595"></a>

Configure a Yum repository properly for subsequent installation of the required software and dependencies.

1. Mount the system image. This document uses openEuler as an example.

    ```bash
    mount YOUR_OS.iso /mnt -o loop
    ```

    >![](public_sys-resources/icon-note.gif) **Note:**
    >`YOUR_OS.iso` indicates the image file of the OS in your environment.

2. Configure a local Yum repository.
    1. Create and open the `/etc/yum.repos.d/openEuler.repo` file.

        ```bash
        vim /etc/yum.repos.d/openEuler.repo
        ```

        >![](public_sys-resources/icon-note.gif) **Note:**
        >The `openEuler.repo` file needs to be manually created. You are advised to back up original `.repo` files.

    2. Press `i` to enter the insert mode and add the following content to the `openEuler.repo` file:

        ```bash
        [openEuler]
        name=openEuler
        baseurl=file:///mnt
        enabled=1
        gpgcheck=0
        ```

    3. Press `Esc`, type `:wq!`, and press `Enter` to save the file and exit.

3. Make the Yum repository configuration take effect.

    ```bash
    yum clean all
    yum makecache
    ```

### Installing Ragel<a name="EN-US_TOPIC_0000002518785772"></a>

Hyperscan compilation depends on Ragel. In this document, Ragel 6.10 is used in the compilation environment.

1. Obtain the Ragel 6.10 source package.

    ```bash
    wget http://www.colm.net/files/ragel/ragel-6.10.tar.gz
    ```

    >![](public_sys-resources/icon-note.gif) **Note:**
    >If the server cannot connect to the Internet, you can download the software package to the local PC and then upload it to the server. For details about the software package download address, see [**Table 1**](#software-requirements).

2. Decompress the source package.

    ```bash
    tar -xzf ragel-6.10.tar.gz
    ```

3. Access the Ragel source code directory.

    ```bash
    cd ./ragel-6.10
    ```

4. Compile and install Ragel.

    ```bash
    ./configure
    make
    make install
    ```

5. Check the Ragel version to verify that the Ragel is successfully installed.

    ```bash
    ragel -v
    ```

    If `Ragel State Machine Compiler version 6.10 March 2017` is displayed, the installation is successful.

### Configuring Boost<a name="EN-US_TOPIC_0000002518785768"></a>

Hyperscan compilation requires Boost 1.57 or later. In this document, Boost 1.87 is used. The following are two methods for configuring Boost. Select one as required.

- Method 1: Download the Boost software package and create a symbolic link. This method does not require Boost installation.
- Method 2: Download the Boost software package and install it on the server. This method does not require symbolic link creation.

The detailed configuration steps are as follows.

**(Method 1) Downloading the Software Package and Creating a Symbolic Link<a name="section194461025186"></a>**

1. Obtain the Boost 1.87 source package.

    ```bash
    wget https://archives.boost.io/release/1.87.0/source/boost_1_87_0.tar.gz
    ```

2. Decompress the source package.

    ```bash
    tar -zxf boost_1_87_0.tar.gz
    ```

3. Create a symbolic link.

    ```bash
    ln -s {boost_path}/boost include/boost
    ```

    >![](public_sys-resources/icon-note.gif) **Note:** 
    >The compilation depends on the Boost header file. `{boost_path}` indicates the full path after `boost_1_87_0.tar.gz` is decompressed. It is recommended that `boost_path` be set to an absolute path.

**(Method 2) Downloading and Installing the Software Package<a name="section1857344614147"></a>**

1. Obtain the software package.

    ```bash
    wget https://archives.boost.io/release/1.87.0/source/boost_1_87_0.tar.gz
    ```

2. Decompress the software package.

    ```bash
    tar -zxf boost_1_87_0.tar.gz
    ```

3. Go to the directory generated after the decompression.

    ```bash
    cd boost_1_87_0
    ```

4. Run the `bootstrap.sh` script and set related parameters.

    ```bash
    ./bootstrap.sh --with-libraries=all --with-toolset=gcc
    ```

5. Perform compilation.

    ```bash
    ./b2 toolset=gcc
    ```

6. Install Boost.

    ```bash
    ./b2 install --prefix=/usr
    ```

    If information similar to the following is displayed, the installation is successful.

    ![](figures/en-us_image_0000002518785776.png)

7. Update the dynamic link libraries in the system.

    ```bash
    ldconfig
    ```

### Downloading PCRE<a name="EN-US_TOPIC_0000002518625864"></a>

The compilation of the Hyperscan tool hscollider depends on PCRE 8.41 or later. This document uses PCRE 8.43 as an example.

1. Obtain the PCRE 8.43 source package as instructed in [**Table 1**](#software-requirements).
2. Decompress the source package.

    ```bash
    tar -zxf pcre-8.43.tar.gz
    ```

### Installing SQLite<a name="EN-US_TOPIC_0000002518785774"></a>

The compilation of the Hyperscan tool hsbench depends on SQLite 3. Run the `yum` command to install SQLite and the SQLite development suite. After the installation is complete, check the SQLite version.

1. Install SQLite and the SQLite development suite.

    ```bash
    yum install sqlite sqlite-devel
    ```

2. After the installation is complete, run the following command to check whether the development suite is properly configured:

    ```bash
    pkg-config --libs sqlite3
    ```

    - If yes, the following information is displayed.

        ```bash
        -lsqlite3
        ```

    - If not, perform the following steps:
        1. Open the `/usr/lib64/pkgconfig/sqlite3.pc` file.

            ```bash
            vim /usr/lib64/pkgconfig/sqlite3.pc
            ```

        2. Press `i` to enter the insert mode and add the following content to the `/usr/lib64/pkgconfig/sqlite3.pc` file:

            ```bash
            # Package Information for pkg-config
            
            prefix=/usr
            exec_prefix=/usr
            libdir=/usr/lib64
            includedir=/usr/include
            
            Name: SQLite
            Description: SQL database engine
            Version: 3.37.2
            Libs: -L${libdir} -lsqlite3
            Libs.private: -lm -lz
            Cflags: -I${includedir}
            ```

            >![](public_sys-resources/icon-note.gif) **Note:**
            >Replace `libdir` and `includedir` with the actual installation paths.

        3. Press `Esc`, type `:wq!`, and press `Enter` to save the file and exit.

### Installing KHSEL<a name="EN-US_TOPIC_0000002550345613"></a>

KHSEL is a Hyperscan enhancement package used to improve the scan performance of Hyperscan. The KHSEL source code has been integrated into the `dev` branch on the GitCode platform, which is stored in the `src\kunpeng-enhanced` directory. Therefore, you do not need to install the KHSEL software package separately.

## Compiling Hyperscan<a name="EN-US_TOPIC_0000002518625866"></a>

In the Hyperscan source code directory, add the PCRE dependency library, and compile the source code in static library, dynamic library, or debug mode.

1. Go to the Hyperscan source code directory.

    ```bash
    cd hyperscan
    ```

2. Add the PCRE dependency library.

    The compilation of the hscollider tool source code depends on the PCRE tool.

    1. Go to the PCRE 8.43 download directory, extract and copy the `pcre-8.43` folder to the Hyperscan source code directory, and rename the folder as `pcre`.

        ```bash
        cp -rf ./pcre-8.43 hyperscan/pcre
        ```

    2. Open the `pcre/CMakeLists.txt` file.

        ```bash
        vim hyperscan/pcre/CMakeLists.txt
        ```

    3. Press `i` to enter the insert mode and comment out line 77 in the copied `pcre/CMakeLists.txt` file as follows:

        ```bash
        CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0)
        #CMAKE_POLICY(SET CMP0026 OLD)
        ```

        In the `CMakeLists.txt` file, the command in line 77 cannot be identified in CMake versions earlier than 2.8.1 and does not affect functionality. The command needs to be commented out. You can also upgrade CMake to 3.0 or later to solve the problem that the `CMAKE_POLICY` command cannot be identified.

    4. Press `Esc`, type `:wq!`, and press `Enter` to save the file and exit.

3. Compile the source code.
    1. Go to the Hyperscan source code directory.

        ```bash
        cd hyperscan
        ```

    2. Create a `build` directory.

        ```bash
        mkdir -p build
        ```

    3. Go to the created `build` directory.

        ```bash
        cd build
        ```

    4. Perform compilation.

        The compilation mode can be release or debug, and the compilation results can be static or dynamic libraries. By default, the release mode is used and static libraries are generated. If dynamic libraries are required, add the corresponding compilation option.

        - Compile static libraries from the source code. By default, static libraries are generated in release mode.

            ```bash
            cmake ..
            make -j
            ```

            After the compilation, the following static libraries and test programs of Hyperscan are generated by default:

            ![](figures/en-us_image_0000002550305603.png)

            Generated test programs:

            ![](figures/1.png)

            Generated static libraries:

            ![](figures/2.png)

        - Compile dynamic libraries from the source code. Add the `-DBUILD_SHARED_LIBS=ON` option to the compile command.

            ```bash
            cmake .. -DBUILD_SHARED_LIBS=ON
            ```

            Generated dynamic libraries:

            ![](figures/3.png)

        - Compile the source code in debug mode. Add the `-DCMAKE_BUILD_TYPE=DEBUG` option to the compile command.

            ```bash
            cmake .. -DCMAKE_BUILD_TYPE=DEBUG
            ```

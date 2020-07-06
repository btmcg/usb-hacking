usb-hacking
===========

Tools and experimental code to use USB on Linux.


Cloning repo and submodules
---------------------------

.. code-block::

   git clone --recursive https://gitlab.com/btmcg/protocol-parser.git

or

.. code-block::

   git clone https://gitlab.com/btmcg/protocol-parser.git
   git submodule update --init --recursive


Building and running
--------------------

build options
~~~~~~~~~~~~~

The default compiler is set to gcc. To build with clang, use

    ``make COMPILER=clang -j``

To build with debugging assertions built in, use

    ``make DEBUG=1 -j``

For example, to build only the tests, using clang, and with debug code,
use

    ``make DEBUG=1 COMPILER=clang test -j``


Installing and maintaining third-party libraries
------------------------------------------------

google benchmark v1.5.1
~~~~~~~~~~~~~~~~~~~~~~~

**Build and install**

.. code-block::

    # from protocol-parser root
    git clone --branch=v1.5.1 --depth=1 https://github.com/google/benchmark.git gb
    cd gb

    cmake . -DBENCHMARK_ENABLE_GTEST_TESTS=OFF -DBENCHMARK_ENABLE_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DCMAKE_INSTALL_PREFIX=../third_party/google-benchmark-gcc/1.5.1
    make install -j

    # now build with clang
    rm -rf CMakeFiles src/generated

    cmake . -DBENCHMARK_ENABLE_GTEST_TESTS=OFF -DBENCHMARK_ENABLE_TESTING=OFF -DBENCHMARK_USE_LIBCXX=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DCMAKE_INSTALL_PREFIX=../third_party/google-benchmark-clang/1.5.1
    make install -j

    cd ..
    rm -rf gb


catch
~~~~~

**Add new submodule**

.. code-block::

    git submodule add -- https://github.com/catchorg/Catch2.git third_party/catch2/2.12.2
    cd third_party/catch2/2.12.2
    git checkout v2.12.2


**Remove old submodule**

.. code-block::

    vim .gitmodules
    vim .git/config
    git add .gitmodules
    git rm --cached third_party/catch2/2.12.1
    rm -rf .git/modules/third_party/catch2/2.12.1
    rm -rf third_party/catch2/2.12.1

**Point makefile to new version**

.. code-block::

    vim nrmake/third_party.mk


fmt
~~~

**Add new submodule**

.. code-block::

    git submodule add -- https://github.com/fmtlib/fmt.git third_party/fmt/7.0.0
    cd third_party/fmt/7.0.0
    git checkout 7.0.0

**Remove old submodule**

.. code-block::

    vim .gitmodules
    vim .git/config
    git add .gitmodules
    git rm --cached third_party/fmt/6.2.1
    rm -rf .git/modules/third_party/fmt/6.2.1
    rm -rf third_party/fmt/6.2.1

**Point makefile to new version**

.. code-block::

    vim nrmake/third_party.mk

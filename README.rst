###########
usb-hacking
###########

Tools and experimental code to interface with USB on Linux.

``led-ctl``
    An application that controls a `Delcom USB HID Visual Signal
    Indicator
    <https://www.delcomproducts.com/productdetails.asp?PartNumber=904000-S>`_.
    Uses `libusb <https://libusb.info/>`_.

``lsusb2``
    Lists properties and attributes of currently-connected USB devices.
    This is a custom implementation using `libusb
    <https://libusb.info/>`_ that mimics the behavior of ``lsusb``.



Cloning repo and submodules
===========================

.. code-block::

   git clone --recursive https://gitlab.com/btmcg/usb-hacking.git

or

.. code-block::

   git clone https://gitlab.com/btmcg/usb-hacking.git
   git submodule update --init --recursive


Building and running
====================

build options
-------------

``make -j``
    Using gcc, build all targets with full optimizations, including
    benchmarks and tests (if available).

``make COMPILER=clang -j``
    Using clang, build all targets with full optimizations, including
    benchmarks and tests (if available).

``make DEBUG=1 -j``
    Using gcc, build all targets with no optimizations and debug
    assertions included.

``make DEBUG=1 COMPILER=clang test -j``
    Using clang, build only the test target (and dependencies) with no
    optimizations and debug assertions included.


Development
===========

See `README.dev <README.dev.rst>`_.

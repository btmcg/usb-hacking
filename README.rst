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

The default compiler is set to gcc. To build with clang, use

    ``make COMPILER=clang -j``

To build with debugging assertions built in, use

    ``make DEBUG=1 -j``

For example, to build only the tests, using clang, and with debug code,
use

    ``make DEBUG=1 COMPILER=clang test -j``


Development
===========

See `README.dev <README.dev.rst>`_.

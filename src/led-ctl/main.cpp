#include "arg_parse.hpp"
#include <fmt/core.h>
#include <libusb.h>
#include <cstdint>
#include <cstdlib>
#include <limits>


int
main(int argc, char** argv)
{
    cli_args const args = arg_parse(argc, argv);
    if (args.vendor_id < 0 || args.vendor_id > std::numeric_limits<std::uint16_t>::max()) {
        fmt::print(stderr, "error: invalid vendor id\n");
        return EXIT_FAILURE;
    }
    if (args.product_id < 0 || args.product_id > std::numeric_limits<std::uint16_t>::max()) {
        fmt::print(stderr, "error: invalid product id\n");
        return EXIT_FAILURE;
    }

    libusb_context* ctx = nullptr;
    if (int rv = ::libusb_init(&ctx); rv != 0) {
        fmt::print(stderr, "libusb_init: failure ({})\n",
                ::libusb_strerror(static_cast<libusb_error>(rv)));
        std::exit(EXIT_FAILURE);
    }

    if (args.debug) {
        if (int rv = ::libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
                rv != 0) {
            fmt::print(stderr, "libusb_set_option: failure ({})\n",
                    ::libusb_strerror(static_cast<libusb_error>(rv)));
            ::libusb_exit(ctx);
            std::exit(EXIT_FAILURE);
        }
    }

    libusb_device_handle* dev_handle
            = ::libusb_open_device_with_vid_pid(ctx, static_cast<std::uint16_t>(args.vendor_id),
                    static_cast<std::uint16_t>(args.product_id));
    if (dev_handle == nullptr) {
        fmt::print(stderr, "libusb: device could not be found or permission denied\n");
        ::libusb_exit(ctx);
        std::exit(EXIT_FAILURE);
    }

    if (int kernel_attached = ::libusb_kernel_driver_active(dev_handle, 0); kernel_attached == 0) {
        fmt::print("no kernel driver attached\n");
    } else if (kernel_attached == 1) {
        if (int rv = ::libusb_detach_kernel_driver(dev_handle, 0); rv != 0) {
            fmt::print(stderr, "libusb: device could not be found or permission denied\n");
            ::libusb_close(dev_handle);
            ::libusb_exit(ctx);
            std::exit(EXIT_FAILURE);
        }
    } else {
        fmt::print(stderr, "libusb_kernel_driver_active: failure ({})\n",
                ::libusb_strerror(static_cast<libusb_error>(kernel_attached)));
        ::libusb_close(dev_handle);
        ::libusb_exit(ctx);
        std::exit(EXIT_FAILURE);
    }

    if (int rv = ::libusb_claim_interface(dev_handle, /*interface=*/0); rv != 0) {
        fmt::print(stderr, "libusb_claim_interface: failure ({})\n",
                ::libusb_strerror(static_cast<libusb_error>(rv)));
        ::libusb_close(dev_handle);
        ::libusb_exit(ctx);
        std::exit(EXIT_FAILURE);
    }


    std::uint8_t buf[8] = {};
    buf[0] = 101; // major cmd (8-byte write command)
    buf[1] = 2; // minor cmd (write port 1)
    buf[2] = 0x03; // lsb (port 1 value of 0x03)

    fmt::print("send: [");
    for (int i = 0; i < 8; ++i) {
        fmt::print("{:#04x} ", buf[i]);
    }
    fmt::print("]\n");

    for (int i = 0; i < 1000; ++i) {
        if (int rv = ::libusb_control_transfer(dev_handle,
                    LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE | LIBUSB_ENDPOINT_OUT,
                    0x09 /*HID Set_Report*/, (3 /*HID feature*/ << 8) | buf[0],
                    /*interface=*/0, buf, sizeof(buf), 1000 /*timeout millis*/);
                rv <= 0) {
            fmt::print(stderr, "libusb_control_transfer: failure ({})\n",
                    ::libusb_strerror(static_cast<libusb_error>(rv)));
        }
    }

    fmt::print("recv: [");
    for (int i = 0; i < 8; ++i) {
        fmt::print("{:#04x} ", buf[i]);
    }
    fmt::print("]\n");


    //
    // shutdown/disconnect sequence
    //

    if (int rv = ::libusb_release_interface(dev_handle, /*interface=*/0); rv != 0) {
        fmt::print(stderr, "libusb_release_interface: failure ({})\n",
                ::libusb_strerror(static_cast<libusb_error>(rv)));
        ::libusb_close(dev_handle);
        ::libusb_exit(ctx);
        std::exit(EXIT_FAILURE);
    }

    ::libusb_close(dev_handle);
    ::libusb_exit(ctx);
    return EXIT_SUCCESS;
}

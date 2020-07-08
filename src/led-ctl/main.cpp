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

    fmt::print("vendor_id={:#06x} ({})\n"
               "product_id={:#06x} ({})\n",
            args.vendor_id, args.vendor_id, args.product_id, args.product_id);

    libusb_context* ctx = nullptr;
    if (int rv = ::libusb_init(&ctx); rv != 0) {
        fmt::print(stderr, "libusb_init: failure ({})\n",
                ::libusb_strerror(static_cast<libusb_error>(rv)));
        std::exit(EXIT_FAILURE);
    }

    if (int rv = ::libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
            rv != 0) {
        fmt::print(stderr, "libusb_set_option: failure ({})\n",
                ::libusb_strerror(static_cast<libusb_error>(rv)));
        ::libusb_exit(ctx);
        std::exit(EXIT_FAILURE);
    }

    libusb_device_handle* dev_handle
            = ::libusb_open_device_with_vid_pid(ctx, static_cast<std::uint16_t>(args.vendor_id),
                    static_cast<std::uint16_t>(args.product_id));
    if (dev_handle == nullptr) {
        fmt::print(stderr, "libusb: device could not be found or permission denied\n");
        ::libusb_exit(ctx);
        std::exit(EXIT_FAILURE);
    }

    int const kernel_attached = ::libusb_kernel_driver_active(dev_handle, 0);
    if (kernel_attached == 0) {
        fmt::print("no kernel driver attached\n");
    } else if (kernel_attached == 1) {
        fmt::print("kernel is attached\n");
    } else {
        fmt::print(stderr, "libusb_kernel_driver_active: failure ({})\n",
                ::libusb_strerror(static_cast<libusb_error>(kernel_attached)));
        ::libusb_close(dev_handle);
        ::libusb_exit(ctx);
        std::exit(EXIT_FAILURE);
    }

    // ::libusb_detach_kernel_driver(dev_handle, 0)

    ::libusb_close(dev_handle);
    ::libusb_exit(ctx);
    return EXIT_SUCCESS;
}

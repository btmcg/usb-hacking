#include "arg_parse.hpp"
#include "delcom.hpp"
#include <fmt/core.h>
#include <libusb.h>
#include <cstdint>
#include <cstdlib>
#include <limits>


libusb_device_handle* open_device(libusb_context* const ctx, std::uint16_t vid, std::uint16_t pid);


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
        fmt::print(stderr, "libusb_init failure ({})\n",
                ::libusb_strerror(static_cast<libusb_error>(rv)));
        std::exit(EXIT_FAILURE);
    }

    if (args.debug) {
        if (int rv = ::libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
                rv != 0) {
            fmt::print(stderr, "libusb_set_option failure ({})\n",
                    ::libusb_strerror(static_cast<libusb_error>(rv)));
            ::libusb_exit(ctx);
            std::exit(EXIT_FAILURE);
        }
    }

    libusb_device_handle* dev = open_device(ctx, args.vendor_id, args.product_id);
    if (dev == nullptr) {
        fmt::print(stderr, "device {:04x}:{:04x} not found\n", args.vendor_id, args.product_id);
        ::libusb_exit(ctx);
        std::exit(EXIT_FAILURE);
    }

    try {
        delcom::vi_hid hid(dev);
        delcom::firmware_info const info = hid.get_firmware_info();
        fmt::print("firmware: serial_number={},version={},date={}{:02}{:02}\n", info.serial_number,
                info.version, info.year, info.month, info.day);

        hid.flash_led(delcom::Color::Red);
        hid.flash_led(delcom::Color::Green);
        hid.flash_led(delcom::Color::Blue);
    } catch (std::exception const& e) {
        fmt::print(stderr, "exception: {}\n", e.what());
        ::libusb_close(dev);
        ::libusb_exit(ctx);
        std::exit(EXIT_FAILURE);
    }

    ::libusb_close(dev);
    ::libusb_exit(ctx);
    return EXIT_SUCCESS;
}


libusb_device_handle*
open_device(libusb_context* const ctx, std::uint16_t vid, std::uint16_t pid)
{
    libusb_device** devices = nullptr;
    ssize_t num_devs = ::libusb_get_device_list(ctx, &devices);
    if (num_devs < 0) {
        fmt::print(stderr, "libusb_get_device_list failure ({})\n",
                ::libusb_strerror(static_cast<libusb_error>(num_devs)));
        ::libusb_free_device_list(devices, 1);
        return nullptr;
    }

    libusb_device* dev = nullptr;
    for (ssize_t i = 0; i < num_devs; ++i) {
        libusb_device_descriptor dd;
        if (int rv = ::libusb_get_device_descriptor(devices[i], &dd); rv != 0) {
            fmt::print(stderr, "libusb_get_device_descriptor failure ({})\n",
                    ::libusb_strerror(static_cast<libusb_error>(num_devs)));
            ::libusb_free_device_list(devices, 1);
            return nullptr;
        }

        if (dd.idVendor == vid && dd.idProduct == pid) {
            dev = devices[i];
            break;
        }
    }

    libusb_device_handle* dev_handle = nullptr;
    if (int rv = ::libusb_open(dev, &dev_handle); rv != 0) {
        fmt::print(stderr, "libusb_open failure ({})\n",
                ::libusb_strerror(static_cast<libusb_error>(rv)));
        dev = nullptr;
        dev_handle = nullptr;
    }

    // decrements all device counts by 1
    ::libusb_free_device_list(devices, 1);
    return dev_handle;
}

#include "arg_parse.hpp"
#include "delcom.hpp"
#include "util/assert.hpp"
#include <fmt/core.h>
#include <libusb.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <thread> // std::this_thread


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
        fmt::print(
                stderr, "failed to open device {:04x}:{:04x}\n", args.vendor_id, args.product_id);
        ::libusb_exit(ctx);
        std::exit(EXIT_FAILURE);
    }

    int exit_code = EXIT_SUCCESS;
    try {
        using delcom::Color;
        delcom::vi_hid hid(dev);
        fmt::print("{}\n", hid.read_firmware_info().str());
        fmt::print("\n[{}]\n", hid.read_ports_and_pins().str());

        hid.turn_led_on(Color::Green);
        hid.turn_led_on(Color::Red);
        hid.turn_led_on(Color::Blue);

        fmt::print("\n[{}]\n", hid.read_ports_and_pins().str());
    } catch (std::exception const& e) {
        fmt::print(stderr, "exception: {}\n", e.what());
        exit_code = EXIT_FAILURE;
    }

    ::libusb_close(dev);
    ::libusb_exit(ctx);
    return exit_code;
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
    if (dev != nullptr) {
        if (int rv = ::libusb_open(dev, &dev_handle); rv != 0) {
            fmt::print(stderr, "libusb_open failure ({})\n",
                    ::libusb_strerror(static_cast<libusb_error>(rv)));
            dev = nullptr;
            dev_handle = nullptr;
        }
    }

    // decrements all device counts by 1
    ::libusb_free_device_list(devices, 1);
    return dev_handle;
}

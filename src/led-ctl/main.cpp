#include "arg_parse.hpp"
#include "delcom.hpp"
#include "util/assert.hpp"
#include <fmt/core.h>
#include <fmt/ostream.h> // for formatting std::thread_id
#include <libusb.h>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <thread> // std::this_thread


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

    int exit_code = EXIT_SUCCESS;
    try {
        using delcom::Color;

        delcom::vi_hid hid(args.vendor_id, args.product_id, args.debug);
        fmt::print("connected to device {:#06x}:{:#06x} ({})\n", hid.vendor_id(), hid.product_id(),
                hid.read_firmware_info().str());
        fmt::print("device state: [{}]\n", hid.read_port_data().str());

        hid.turn_led_on(Color::Green);

        fmt::print("device state: [{}]\n", hid.read_port_data().str());

    } catch (std::exception const& e) {
        fmt::print(stderr, "exception: {}\n", e.what());
        exit_code = EXIT_FAILURE;
    }

    return exit_code;
}

#pragma once

#include "version.h"
#include "util/compiler.hpp"
#include <filesystem>
#include <getopt.h>
#include <cstdio>  // std::fprintf
#include <cstdlib> // std::exit
#include <string>


struct cli_args
{
    int vendor_id = -1;
    int product_id = -1;
    bool debug = false;
};

cli_args
arg_parse(int argc, char** argv)
{
    auto usage = [](std::FILE* outerr, std::filesystem::path const& app) {
        std::fprintf(outerr,
                "usage: %s [-Dhv] [--device=<vendor_id>:<product_id>]\n"
                "options:\n"
                "  -d, --device=<vendor_id>:<product_id>    Show only devices with the specified vendor and \n"
                "                                           product ID. Both IDs are given in hex (e.g. 0x1234:0xabcd).\n"
                "  -D, --debug                              Enable libusb debugging to stderr.\n"
                "  -h, --help                               This output.\n"
                "  -v, --version                            Print application version information.\n",
                app.c_str());
        std::exit(outerr == stdout ? EXIT_SUCCESS : EXIT_FAILURE);
    };

    auto const app = std::filesystem::path(argv[0]).filename();

    cli_args args;
    while (true) {
        // clang-format off
        static option const long_options[] = {
                { "debug",      no_argument,        nullptr,    'D' },
                { "device",     required_argument,  nullptr,    'd' },
                { "help",       no_argument,        nullptr,    'h' },
                { "version",    no_argument,        nullptr,    'v' },
                { nullptr,      0,                  nullptr,    0 },
        };
        // clang-format on

        int const c = ::getopt_long(
                argc, argv, "d:Dhv", static_cast<option const*>(long_options), nullptr);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                usage(stdout, app);
                break;

            case 'd': {
                std::string vidpid = optarg;
                std::string::size_type const colon = vidpid.find(':');
                if (colon == std::string::npos) {
                    std::fprintf(stderr, "invalid vendor_id:product_id \"%s\"\n", vidpid.c_str());
                    usage(stderr, app);
                }

                args.vendor_id = std::stoi(vidpid.substr(0, colon), nullptr, 16);
                args.product_id = std::stoi(vidpid.substr(colon + 1), nullptr, 16);
                break;
            }

            case 'D':
                args.debug = true;
                break;

            case 'v':
                std::fprintf(stdout, "app_version=%s\n%s\n", ::VERSION,
                        get_version_info_multiline().c_str());
                std::exit(EXIT_SUCCESS);
                break;

            case '?':
            default:
                usage(stderr, app);
                break;
        }
    } // while

    for (; optind != argc; ++optind) {
        std::fprintf(stderr, "extra argument(s): %s\n\n", argv[optind]);
        usage(stderr, app);
    }

    return args;
}

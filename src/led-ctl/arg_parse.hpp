#pragma once

#include "version.hpp"
#include "util/compiler.hpp"
#include <filesystem>
#include <getopt.h>
#include <cstdio> // std::fprintf
#include <cstdlib> // std::exit
#include <string>


struct cli_args
{
    int vendor_id = -1;
    int product_id = -1;
};

cli_args
arg_parse(int argc, char** argv)
{
    auto usage = [](std::FILE* outerr, std::filesystem::path const& app) {
        std::fprintf(outerr,
                "usage: %s [-hv] <vendor_id>:<product_id>\n"
                "arguments:\n"
                "   vendor_id               usb vendor id (in hex (e.g. 0x0123)\n"
                "   product_id               usb device id (in hex (e.g. 0x3210)\n"
                "options:\n"
                "  -h, --help               this output\n"
                "  -v, --version            version\n",
                app.c_str());
        std::exit(outerr == stdout ? EXIT_SUCCESS : EXIT_FAILURE);
    };

    auto const app = std::filesystem::path(argv[0]).filename();

    cli_args args;
    while (true) {
        static option long_options[] = {
                {"help", no_argument, nullptr, 'h'},
                {"version", no_argument, nullptr, 'v'},
                {nullptr, 0, nullptr, 0},
        };

        int const c = ::getopt_long(
                argc, argv, "hv", static_cast<option const*>(long_options), nullptr);
        if (c == -1)
            break;

        switch (c) {
            case 'h':
                usage(stdout, app);
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

    if (optind == argc) {
        std::fprintf(stderr, "missing required argument(s)\n\n");
        usage(stderr, app);
    }

    std::string vdid;
    for (; optind != argc; ++optind) {
        if (vdid.empty()) {
            vdid = argv[optind];
        } else {
            std::fprintf(stderr, "extra argument(s): %s\n\n", argv[optind]);
            usage(stderr, app);
        }
    }

    std::string::size_type const colon = vdid.find(':');
    if (colon == std::string::npos)
        return args;

    args.vendor_id = std::stoi(vdid.substr(0, colon), nullptr, 16);
    args.product_id = std::stoi(vdid.substr(colon + 1), nullptr, 16);
    return args;
}

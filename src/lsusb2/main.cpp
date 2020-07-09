#include "arg_parse.hpp"
#include "enums.hpp"
#include "util/assert.hpp"
#include <fmt/format.h>
#include <libusb.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>


bool print_device_desc(libusb_device*);
bool print_config_desc(libusb_config_descriptor const*);
bool print_interface_desc(libusb_interface_descriptor const*);
bool print_endpoint_desc(libusb_endpoint_descriptor const*);


/**********************************************************************/


int
main(int argc, char** argv)
{
    cli_args const args = arg_parse(argc, argv);

    std::uint16_t target_vid = 0;
    std::uint16_t target_pid = 0;
    if (args.vendor_id != -1) {
        if (args.vendor_id < 0 || args.vendor_id > std::numeric_limits<std::uint16_t>::max()) {
            fmt::print(stderr, "error: invalid vendor id: {:#06x}\n", args.vendor_id);
            return EXIT_FAILURE;
        }
        if (args.product_id < 0 || args.product_id > std::numeric_limits<std::uint16_t>::max()) {
            fmt::print(stderr, "error: invalid product id: {:#06x}\n", args.product_id);
            return EXIT_FAILURE;
        }

        target_vid = args.vendor_id;
        target_pid = args.product_id;
    }

    libusb_context* ctx = nullptr;
    if (int rv = ::libusb_init(&ctx); rv != 0) {
        fmt::print(stderr, "libusb_init: failure ({})\n",
                ::libusb_strerror(static_cast<libusb_error>(rv)));
        std::exit(EXIT_FAILURE);
    }

    if (args.debug) {
        int rv = ::libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
        if (rv != 0) {
            fmt::print(stderr, "libusb_get_port_numbers failure ({})\n",
                    ::libusb_strerror(static_cast<libusb_error>(rv)));
            ::libusb_exit(ctx);
            std::exit(EXIT_FAILURE);
        }
    }

    libusb_device** devices = nullptr;
    ssize_t num_devs = ::libusb_get_device_list(ctx, &devices);
    if (num_devs < 0) {
        fmt::print(stderr, "libusb_get_device_list failure ({})\n",
                ::libusb_strerror(static_cast<libusb_error>(num_devs)));
        ::libusb_free_device_list(devices, 1);
        ::libusb_exit(ctx);
        std::exit(EXIT_FAILURE);
    }

    if (target_vid != 0) {
        // print only device that matches vendor_id:product_id
        bool dev_found = false;
        for (ssize_t i = 0; i < num_devs; ++i) {
            libusb_device_descriptor dd;
            if (int rv = ::libusb_get_device_descriptor(devices[i], &dd); rv != 0) {
                fmt::print(stderr, "libusb_get_device_descriptor failure ({})\n",
                        ::libusb_strerror(static_cast<libusb_error>(num_devs)));
                ::libusb_free_device_list(devices, 1);
                ::libusb_exit(ctx);
                std::exit(EXIT_FAILURE);
            }

            if (dd.idVendor == target_vid && dd.idProduct == target_pid) {
                fmt::print("device {}: {:04x}:{:04x}\n", i, target_vid, target_pid);
                if (!print_device_desc(devices[i])) {
                    ::libusb_free_device_list(devices, 1);
                    ::libusb_exit(ctx);
                    std::exit(EXIT_FAILURE);
                }
                dev_found = true;
                break;
            }
        }

        if (!dev_found)
            fmt::print("no device {:04x}:{:04x} found\n", target_vid, target_pid);

    } else {
        // print all devices
        for (ssize_t i = 0; i < num_devs; ++i) {
            fmt::print("device {}:\n", i);
            if (!print_device_desc(devices[i])) {
                ::libusb_exit(ctx);
                ::libusb_free_device_list(devices, 1);
                std::exit(EXIT_FAILURE);
            }
        }
    }

    ::libusb_free_device_list(devices, 1);
    ::libusb_exit(ctx);
    return EXIT_SUCCESS;
}

bool
print_device_desc(libusb_device* dev)
{
    DEBUG_ASSERT(dev != nullptr);
    bool success = true;

    std::uint8_t ports[7];
    int const num_ports = ::libusb_get_port_numbers(dev, ports, sizeof(ports) / sizeof(ports[0]));
    if (num_ports < 0) {
        fmt::print(stderr, "libusb_get_port_numbers failure ({})\n",
                ::libusb_strerror(static_cast<libusb_error>(num_ports)));
    }
    std::string const ports_str = (num_ports == 0)
            ? "<none>"
            : fmt::format("{}", fmt::join(std::begin(ports), std::begin(ports) + num_ports, ","));

    fmt::print("  bus:                {}\n"
               "  address:            {}\n"
               "  port(s):            {}\n"
               "  speed:              {}\n",
            ::libusb_get_bus_number(dev), ::libusb_get_device_address(dev), ports_str,
            to_str(static_cast<libusb_speed>(::libusb_get_device_speed(dev))));

    libusb_device_descriptor dd;
    ::libusb_get_device_descriptor(dev, &dd);

    DEBUG_ASSERT(dd.bLength == 18);
    DEBUG_ASSERT(dd.bDescriptorType == LIBUSB_DT_DEVICE);

    fmt::print("  usb spec release:   {}\n"
               "  class:              {}\n"
               "  subclass:           {}\n"
               "  protocol:           {}\n"
               "  max packet size:    {}\n"
               "  vendor id:          {:#06x}\n"
               "  product id:         {:#06x}\n"
               "  device release:     {}\n"
               "  manufacturer:       {}\n"
               "  product:            {}\n"
               "  serial number:      {}\n"
               "  num configurations: {}\n",
            to_version(dd.bcdUSB), to_str(static_cast<libusb_class_code>(dd.bDeviceClass)),
            to_str(static_cast<libusb_class_code>(dd.bDeviceSubClass)), dd.bDeviceProtocol,
            dd.bMaxPacketSize0, dd.idVendor, dd.idProduct, to_version(dd.bcdDevice),
            dd.iManufacturer, dd.iProduct, dd.iSerialNumber, dd.bNumConfigurations);

    for (int config_num = 0; config_num < dd.bNumConfigurations; ++config_num) {
        libusb_config_descriptor* cd = nullptr;
        if (int rv = ::libusb_get_config_descriptor(dev, config_num, &cd); rv != 0) {
            fmt::print(stderr, "libusb_get_config_descriptor failure ({})\n",
                    ::libusb_strerror(static_cast<libusb_error>(rv)));
            success = false;
            break;
        }

        fmt::print("    configuration {}:\n", config_num);

        if (!print_config_desc(cd)) {
            success = false;
            ::libusb_free_config_descriptor(cd);
            break;
        }

        ::libusb_free_config_descriptor(cd);
    }

    return success;
}

bool
print_config_desc(libusb_config_descriptor const* cd)
{
    DEBUG_ASSERT(cd != nullptr);
    DEBUG_ASSERT(cd->bDescriptorType == LIBUSB_DT_CONFIG);
    DEBUG_ASSERT(cd->bLength == 9);

    fmt::print("      total length:    {}\n"
               "      config value:    {}\n"
               "      config:          {}\n"
               "      attributes:      {}\n"
               "      max power:       {}\n"
               "      unknown configs: {}\n"
               "      num interfaces:  {}\n",
            cd->wTotalLength, cd->bConfigurationValue, cd->iConfiguration, cd->bmAttributes,
            cd->MaxPower, cd->extra_length, cd->bNumInterfaces);

    if (cd->bNumInterfaces > 0) {
        libusb_interface const* iface = cd->interface;

        for (int iface_num = 0; iface_num < iface->num_altsetting; ++iface_num) {
            libusb_interface_descriptor const ifd = iface->altsetting[iface_num];
            fmt::print("        interface {}:\n", iface_num);
            if (!print_interface_desc(&ifd))
                return false;
        }
    }

    return true;
}

bool
print_interface_desc(libusb_interface_descriptor const* ifd)
{
    DEBUG_ASSERT(ifd != nullptr);
    DEBUG_ASSERT(ifd->bDescriptorType == LIBUSB_DT_INTERFACE);
    DEBUG_ASSERT(ifd->bLength == 9);

    fmt::print("          number:             {}\n"
               "          alternate setting:  {}\n"
               "          class:              {}\n"
               "          sub-class:          {}\n"
               "          protocol:           {}\n"
               "          interface index:    {}\n"
               "          unknown interfaces: {}\n"
               "          num endpoints:      {}\n",
            ifd->bInterfaceNumber, ifd->bAlternateSetting,
            to_str(static_cast<libusb_class_code>(ifd->bInterfaceClass)),
            to_str(static_cast<libusb_class_code>(ifd->bInterfaceSubClass)),
            ifd->bInterfaceProtocol, ifd->iInterface, ifd->extra_length, ifd->bNumEndpoints);

    for (int ep_num = 0; ep_num < ifd->bNumEndpoints; ++ep_num) {
        libusb_endpoint_descriptor const epd = ifd->endpoint[ep_num];

        fmt::print("            endpoint {}:\n", ep_num);
        if (!print_endpoint_desc(&epd))
            return false;
    }

    return true;
}

bool
print_endpoint_desc(libusb_endpoint_descriptor const* epd)
{
    DEBUG_ASSERT(epd != nullptr);
    DEBUG_ASSERT(epd->bDescriptorType == LIBUSB_DT_ENDPOINT);
    DEBUG_ASSERT(epd->bLength == 7);

    fmt::print("              address:           {}\n"
               "                number:            {}\n"
               "                direction:         {}\n"
               "              attrs:             {}\n"
               "                transfer type:     {}\n"
               "                iso sync type:     {}\n"
               "                iso usage type:    {}\n"
               "              max packet size:   {} bytes\n"
               "              interval:          {} msec\n"
               "              refresh:           {}\n"
               "              sync address:      {}\n"
               "              unknown endpoints: {}\n",
            epd->bEndpointAddress, ep_addr_to_ep_num(epd->bEndpointAddress),
            to_str(ep_addr_to_endpoint_direction(epd->bEndpointAddress)), epd->bmAttributes,
            to_str(ep_attr_to_transfer_type(epd->bmAttributes)),
            to_str(ep_attr_to_iso_sync_type(epd->bmAttributes)),
            to_str(ep_attr_to_iso_usage_type(epd->bmAttributes)), epd->wMaxPacketSize,
            epd->bInterval, epd->bRefresh, epd->bSynchAddress, epd->extra_length);

    return true;
}

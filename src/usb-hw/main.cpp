#include "util/assert.hpp"
#include <fmt/format.h>
#include <libusb.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>


constexpr char const*
class_code_to_str(libusb_class_code c)
{
    switch (c) {
        // clang-format off
        case LIBUSB_CLASS_PER_INTERFACE:        return "interface-specific";
        case LIBUSB_CLASS_AUDIO:                return "audio";
        case LIBUSB_CLASS_COMM:                 return "communications";
        case LIBUSB_CLASS_HID:                  return "human interface device";
        case LIBUSB_CLASS_PHYSICAL:             return "physical";
        case LIBUSB_CLASS_PRINTER:              return "printer";
        case LIBUSB_CLASS_PTP:                  return "image";
        case LIBUSB_CLASS_MASS_STORAGE:         return "mass storage";
        case LIBUSB_CLASS_HUB:                  return "hub";
        case LIBUSB_CLASS_DATA:                 return "data";
        case LIBUSB_CLASS_SMART_CARD:           return "smart card";
        case LIBUSB_CLASS_CONTENT_SECURITY:     return "content security";
        case LIBUSB_CLASS_VIDEO:                return "video";
        case LIBUSB_CLASS_PERSONAL_HEALTHCARE:  return "personal healthcare";
        case LIBUSB_CLASS_DIAGNOSTIC_DEVICE:    return "diagnostic device";
        case LIBUSB_CLASS_WIRELESS:             return "wireless";
        case LIBUSB_CLASS_APPLICATION:          return "application";
        case LIBUSB_CLASS_VENDOR_SPEC:          return "vendor-specific";
            // clang-format on
    }
    return "unknown";
}

constexpr char const*
direction_str(std::uint8_t addr)
{
    return ((addr >> 7) & 1u) ? "host-to-device" : "device-to-host";
}

constexpr std::uint8_t
addr_num(std::uint8_t addr)
{
    return addr & 0b0000111;
}

constexpr libusb_transfer_type
to_transfer_type(std::uint8_t t)
{
    return static_cast<libusb_transfer_type>(t & 0b0000'0011);
}

constexpr libusb_iso_sync_type
to_iso_sync_type(std::uint8_t t)
{
    return static_cast<libusb_iso_sync_type>(t & 0b0000'1100);
}

constexpr char const*
iso_sync_type_to_str(libusb_iso_sync_type t) noexcept
{
    switch (t) {
        // clang-format off
        case LIBUSB_ISO_SYNC_TYPE_NONE:     return "no synchronization";
        case LIBUSB_ISO_SYNC_TYPE_ASYNC:    return "asynchronous";
        case LIBUSB_ISO_SYNC_TYPE_ADAPTIVE: return "adaptive";
        case LIBUSB_ISO_SYNC_TYPE_SYNC:     return "synchronous";
            // clang-format on
    }

    return "unknown";
}

constexpr libusb_iso_usage_type
to_iso_usage_type(std::uint8_t t)
{
    return static_cast<libusb_iso_usage_type>(t & 0b0011'0000);
}

constexpr char const*
iso_usage_type_to_str(libusb_iso_usage_type t) noexcept
{
    switch (t) {
        // clang-format off
        case LIBUSB_ISO_USAGE_TYPE_DATA:        return "data endpoint";
        case LIBUSB_ISO_USAGE_TYPE_FEEDBACK:    return "feedback endpoint";
        case LIBUSB_ISO_USAGE_TYPE_IMPLICIT:    return "implicit feedback endpoint";
            // clang-format on
    }

    return "unknown";
}

constexpr char const*
transfer_type_to_str(libusb_transfer_type t)
{
    switch (t) {
        // clang-format off
        case LIBUSB_TRANSFER_TYPE_CONTROL:      return "control";
        case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:  return "isochronous";
        case LIBUSB_TRANSFER_TYPE_BULK:         return "bulk";
        case LIBUSB_TRANSFER_TYPE_INTERRUPT:    return "interrupt";
        case LIBUSB_TRANSFER_TYPE_BULK_STREAM:  return "stream";
            // clang-format on
    }
    return "unknown";
}

bool
print_desc(libusb_device* dev)
{
    libusb_device_descriptor dd;
    int rv = ::libusb_get_device_descriptor(dev, &dd);
    if (rv != 0) {
        fmt::print(stderr, "libusb_get_device_descriptor: failure ({})\n", rv);
        return false;
    }

    DEBUG_ASSERT(dd.bLength == 18);
    DEBUG_ASSERT(dd.bDescriptorType == LIBUSB_DT_DEVICE);

    fmt::print("    usb spec release:   {:#x}\n"
               "    class:              {}\n"
               "    subclass:           {}\n"
               "    protocol:           {}\n"
               "    max packet size:    {}\n"
               "    vendor id:          {}\n"
               "    product id:         {}\n"
               "    device release:     {:#x}\n"
               "    manufacturer:       {}\n"
               "    product:            {}\n"
               "    serial number:      {}\n"
               "    num configurations: {}\n",
            dd.bcdUSB, class_code_to_str(static_cast<libusb_class_code>(dd.bDeviceClass)),
            class_code_to_str(static_cast<libusb_class_code>(dd.bDeviceSubClass)),
            dd.bDeviceProtocol, dd.bMaxPacketSize0, dd.idVendor, dd.idProduct, dd.bcdDevice,
            dd.iManufacturer, dd.iProduct, dd.iSerialNumber, dd.bNumConfigurations);

    for (int config_num = 0; config_num < dd.bNumConfigurations; ++config_num) {
        libusb_config_descriptor* cd = nullptr;
        rv = ::libusb_get_config_descriptor(dev, config_num, &cd);
        if (rv != 0) {
            fmt::print(stderr, "libusb_get_config_descriptor failure ({})\n", rv);
            return false;
        }

        DEBUG_ASSERT(cd->bDescriptorType == LIBUSB_DT_CONFIG);
        DEBUG_ASSERT(cd->bLength == 9);
        fmt::print("       configuration {}:\n"
                   "           total length:    {}\n"
                   "           config value:    {}\n"
                   "           config:          {}\n"
                   "           attributes:      {}\n"
                   "           max power:       {}\n"
                   "           unknown configs: {}\n"
                   "           num interfaces:  {}\n",
                config_num, cd->wTotalLength, cd->bConfigurationValue, cd->iConfiguration,
                cd->bmAttributes, cd->MaxPower, cd->extra_length, cd->bNumInterfaces);

        if (cd->bNumInterfaces > 0) {
            libusb_interface const* iface = cd->interface;
            for (int iface_num = 0; iface_num < iface->num_altsetting; ++iface_num) {
                libusb_interface_descriptor const ifd = iface->altsetting[iface_num];

                DEBUG_ASSERT(ifd.bDescriptorType == LIBUSB_DT_INTERFACE);
                DEBUG_ASSERT(ifd.bLength == 9);
                fmt::print("               interface {}:\n"
                           "                   number:             {}\n"
                           "                   alternate setting:  {}\n"
                           "                   class:              {}\n"
                           "                   sub-class:          {}\n"
                           "                   protocol:           {}\n"
                           "                   interface index:    {}\n"
                           "                   unknown interfaces: {}\n"
                           "                   num endpoints:      {}\n",
                        iface_num, ifd.bInterfaceNumber, ifd.bAlternateSetting,
                        class_code_to_str(static_cast<libusb_class_code>(ifd.bInterfaceClass)),
                        class_code_to_str(static_cast<libusb_class_code>(ifd.bInterfaceSubClass)),
                        ifd.bInterfaceProtocol, ifd.iInterface, ifd.extra_length,
                        ifd.bNumEndpoints);

                if (ifd.bNumEndpoints > 0) {
                    for (int ep_num = 0; ep_num < ifd.bNumEndpoints; ++ep_num) {
                        libusb_endpoint_descriptor const epd = ifd.endpoint[ep_num];

                        DEBUG_ASSERT(epd.bDescriptorType == LIBUSB_DT_ENDPOINT);
                        DEBUG_ASSERT(epd.bLength == 7);
                        std::string attrs(transfer_type_to_str(to_transfer_type(epd.bmAttributes)));
                        if (to_transfer_type(epd.bmAttributes)
                                == LIBUSB_TRANSFER_TYPE_ISOCHRONOUS) {
                            attrs.append(",");
                            attrs.append(iso_sync_type_to_str(to_iso_sync_type(epd.bmAttributes)));
                            attrs.append(",");
                            attrs.append(
                                    iso_usage_type_to_str(to_iso_usage_type(epd.bmAttributes)));
                        }
                        fmt::print("                       endpoint {}:\n"
                                   "                           address:           {} ({})\n"
                                   "                           attrs:             {}\n"
                                   "                           max packet size:   {}\n"
                                   "                           interval:          {}\n"
                                   "                           refresh:           {}\n"
                                   "                           sync address:      {}\n"
                                   "                           unknown endpoints: {}\n",
                                ep_num, addr_num(epd.bEndpointAddress),
                                direction_str(epd.bEndpointAddress),
                                transfer_type_to_str(to_transfer_type(epd.bmAttributes)),
                                epd.wMaxPacketSize, epd.bInterval, epd.bRefresh, epd.bSynchAddress,
                                epd.extra_length);
                    }
                }
            }
        }

        ::libusb_free_config_descriptor(cd);
    }

    return true;
}

int
main()
{
    libusb_context* ctx = nullptr;

    int rv = ::libusb_init(&ctx);
    if (rv != 0) {
        fmt::print(stderr, "libusb_init: failure ({})\n", rv);
        std::exit(EXIT_FAILURE);
    }

    // rv = ::libusb_set_option(ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
    // if (rv != 0) {
    //     fmt::print(stderr, "libusb_set_option: failure ({})\n", rv);
    //     ::libusb_exit(ctx);
    //     std::exit(EXIT_FAILURE);
    // }

    libusb_device** devices = nullptr;
    ssize_t num_devs = ::libusb_get_device_list(ctx, &devices);
    if (num_devs < 0) {
        fmt::print(stderr, "libusb_get_device_list: failure ({})\n", num_devs);
        ::libusb_exit(ctx);
        ::libusb_free_device_list(devices, 1);
        std::exit(EXIT_FAILURE);
    }

    fmt::print("found {} devices:\n", num_devs);
    for (ssize_t i = 0; i < num_devs; ++i) {
        fmt::print("device {}:\n", i);
        if (!print_desc(devices[i])) {
            ::libusb_exit(ctx);
            ::libusb_free_device_list(devices, 1);
            std::exit(EXIT_FAILURE);
        }
    }

    ::libusb_free_device_list(devices, 1);
    ::libusb_exit(ctx);
    return EXIT_SUCCESS;
}

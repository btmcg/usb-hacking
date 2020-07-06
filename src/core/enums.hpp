#pragma once

#include <fmt/format.h>
#include <libusb.h>
#include <cstdint>
#include <string>


// enum-to-string functions
constexpr char const* to_str(libusb_class_code) noexcept;
constexpr char const* to_str(libusb_iso_sync_type) noexcept;
constexpr char const* to_str(libusb_iso_usage_type) noexcept;
constexpr char const* to_str(libusb_speed) noexcept;
constexpr char const* to_str(libusb_transfer_type) noexcept;

// endpoint_descriptor helpers
constexpr char const* ep_addr_to_direction_str(std::uint8_t b_endpoint_address) noexcept;
constexpr std::uint8_t ep_addr_to_ep_num(std::uint8_t b_endpoint_address) noexcept;
constexpr libusb_transfer_type ep_attr_to_transfer_type(std::uint8_t bm_attributes) noexcept;
constexpr libusb_iso_sync_type ep_attr_to_iso_sync_type(std::uint8_t bm_attributes) noexcept;
constexpr libusb_iso_usage_type ep_attr_to_iso_usage_type(std::uint8_t bm_attributes) noexcept;

// misc helpers
inline std::string to_version(std::uint16_t v);


/**********************************************************************/


constexpr char const*
to_str(libusb_class_code c) noexcept
{
    // clang-format off
    switch (c) {
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
    }
    // clang-format on

    return "unknown";
}

constexpr char const*
to_str(libusb_iso_sync_type e) noexcept
{
    // clang-format off
    switch (e) {
        case LIBUSB_ISO_SYNC_TYPE_NONE:     return "no synchronization";
        case LIBUSB_ISO_SYNC_TYPE_ASYNC:    return "asynchronous";
        case LIBUSB_ISO_SYNC_TYPE_ADAPTIVE: return "adaptive";
        case LIBUSB_ISO_SYNC_TYPE_SYNC:     return "synchronous";
    }
    // clang-format on

    return "unknown";
}

constexpr char const*
to_str(libusb_iso_usage_type e) noexcept
{
    // clang-format off
    switch (e) {
        case LIBUSB_ISO_USAGE_TYPE_DATA:        return "data endpoint";
        case LIBUSB_ISO_USAGE_TYPE_FEEDBACK:    return "feedback endpoint";
        case LIBUSB_ISO_USAGE_TYPE_IMPLICIT:    return "implicit feedback endpoint";
    }
    // clang-format on
    return "unknown";
}

constexpr char const*
to_str(libusb_speed e) noexcept
{
    // clang-format off
    switch (e) {
        case LIBUSB_SPEED_UNKNOWN:      return "unknown";
        case LIBUSB_SPEED_LOW:          return "low (1.5 MBit/s)";
        case LIBUSB_SPEED_FULL:         return "full (12 MBit/s)";
        case LIBUSB_SPEED_HIGH:         return "high (480 MBit/s)";
        case LIBUSB_SPEED_SUPER:        return "super (5000 MBit/s)";
        case LIBUSB_SPEED_SUPER_PLUS:   return "super plus (10000 MBit/s)";
    }
    // clang-format on

    return "<unknown>";
}

constexpr char const*
to_str(libusb_transfer_type e) noexcept
{
    // clang-format off
    switch (e) {
        case LIBUSB_TRANSFER_TYPE_CONTROL:      return "control";
        case LIBUSB_TRANSFER_TYPE_ISOCHRONOUS:  return "isochronous";
        case LIBUSB_TRANSFER_TYPE_BULK:         return "bulk";
        case LIBUSB_TRANSFER_TYPE_INTERRUPT:    return "interrupt";
        case LIBUSB_TRANSFER_TYPE_BULK_STREAM:  return "stream";
    }
    // clang-format on

    return "unknown";
}

constexpr char const*
ep_addr_to_direction_str(std::uint8_t addr) noexcept
{
    return ((addr >> 7) & 1u) ? "host-to-device" : "device-to-host";
}

constexpr std::uint8_t
ep_addr_to_ep_num(std::uint8_t addr) noexcept
{
    return addr & 0b0000'0111;
}

constexpr libusb_transfer_type
ep_attr_to_transfer_type(std::uint8_t t) noexcept
{
    return static_cast<libusb_transfer_type>(t & 0b0000'0011);
}

constexpr libusb_iso_sync_type
ep_attr_to_iso_sync_type(std::uint8_t t) noexcept
{
    return static_cast<libusb_iso_sync_type>(t & 0b0000'1100);
}

constexpr libusb_iso_usage_type
ep_attr_to_iso_usage_type(std::uint8_t t) noexcept
{
    return static_cast<libusb_iso_usage_type>(t & 0b0011'0000);
}

std::string
to_version(std::uint16_t v)
{
    if (v == 0)
        return "0";

    std::string ver = fmt::format("{:x}", v);
    return ver.insert(1, 1, '.');
}

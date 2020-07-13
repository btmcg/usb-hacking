#pragma once

#include "delcom_protocol.hpp"
#include "usb_hid.hpp"
#include <cstddef> // std::size_t
#include <cstdint>
#include <tuple>


struct libusb_device_handle;

namespace delcom {

    struct firmware_info
    {
        std::uint32_t serial_number = 0;
        int version = 0;
        int year = 0;
        int month = 0;
        int day = 0;
    };

    enum class Color : std::uint8_t
    {
        // clang-format off
        Blue  = 0x03,
        Red   = 0x05,
        Green = 0x06,
        None  = 0x07,
        // clang-format on
    };

    /// Delcom's visual indicator USB HID
    /// \ref vendor id = 0x0fc5
    /// \ref product id = 0xb080
    /// \ref family type id = 2
    /// \ref https://www.delcomproducts.com/productdetails.asp?PartNumber=900000
    class vi_hid
    {
    public:
        static constexpr std::uint16_t vendor_id = 0x0fc5;
        static constexpr std::uint16_t product_id = 0xb080;

    private:
        libusb_device_handle* dev_ = nullptr;
        std::uint16_t interface_ = 0;
        std::uint32_t ctrl_timeout_msec_ = 1000;
        std::size_t flash_duration_ = 500;

    public:
        explicit vi_hid(libusb_device_handle*);
        ~vi_hid() noexcept;

        firmware_info get_firmware_info() const;
        void flash_led(Color) const;

        /// \returns event-counter value and overflow status
        std::tuple<std::uint32_t, bool> read_and_reset_event_counter() const;

    private:
        void power_led(Color, std::size_t duration) const;
        void ctrl_transfer(usb::hid::ClassRequest, packet&) const;
    };

} // namespace delcom
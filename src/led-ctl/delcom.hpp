#pragma once

#include "delcom_protocol.hpp"
#include "usb_hid.hpp"
#include <fmt/format.h>
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

    struct port_data
    {
        bool clock_enabled = false;
        std::uint8_t port0 = 0;
        std::uint8_t port1 = 0;
        std::uint8_t port2 = 0;

        std::string
        str() const
        {
            return fmt::format(
                    "port0={:#04x} ({:b}), port1={:#04x} ({:b}), port2={:#04x} ({:b}), clock_enabled={}",
                    port0, port0, port1, port1, port2, port2, clock_enabled);
        }
    };

    enum class Color : std::uint8_t
    {
        // clang-format off
        Green = 0b110,
        Red   = 0b101,
        Blue  = 0b011,
        None  = 0b111,
        // clang-format on
    };

    constexpr Color
    operator&(Color lhs, Color rhs) noexcept
    {
        return static_cast<Color>(static_cast<std::underlying_type_t<Color>>(lhs)
                & static_cast<std::underlying_type_t<Color>>(rhs));
    }

    constexpr Color
    operator&=(Color& lhs, Color rhs) noexcept
    {
        lhs = static_cast<Color>(static_cast<std::underlying_type_t<Color>>(lhs)
                & static_cast<std::underlying_type_t<Color>>(rhs));
        return lhs;
    }

    constexpr Color
    operator|(Color lhs, Color rhs) noexcept
    {
        return static_cast<Color>(static_cast<std::underlying_type_t<Color>>(lhs)
                | static_cast<std::underlying_type_t<Color>>(rhs));
    }

    constexpr Color
    operator|=(Color& lhs, Color rhs) noexcept
    {
        lhs = static_cast<Color>(static_cast<std::underlying_type_t<Color>>(lhs)
                | static_cast<std::underlying_type_t<Color>>(rhs));
        return lhs;
    }

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

        firmware_info read_firmware_info() const;
        port_data read_port_data() const;
        void flash_led(Color) const;

        /// duty_on/duty_off specify time in msecs
        void set_duty_cycle(Color, std::uint8_t duty_on, std::uint8_t duty_off) const;
        void set_pwm(Color, std::uint8_t pct) const;
        void enable_clock(Color) const;
        void disable_clock(Color) const;

        /// \returns event-counter value and overflow status
        std::tuple<std::uint32_t, bool> read_and_reset_event_counter() const;

    private:
        void power_led(Color, std::size_t duration) const;
        void ctrl_transfer(usb::hid::ClassRequest, packet&) const;
    };

} // namespace delcom

#pragma once

#include "protocol.hpp"
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

        std::string
        str() const
        {
            return fmt::format("serial_number={},version={},date={}{:02}{:02}", serial_number,
                    version, year, month, day);
        }
    };

    struct port_data
    {
        std::uint8_t port0 = 0;
        std::uint8_t port1 = 0;
        std::uint8_t port2 = 0;
        std::uint8_t clock_status = 0;

        std::string
        str() const
        {
            return fmt::format("port0={:08b}, port1={:08b}, port2={:08b}, clock_status={:08b}",
                    port0, port1, port2, clock_status);
        }
    };

    enum class Color : std::uint8_t
    {
        // clang-format off
        Green = 0b001,  // port 1, pin 0
        Red   = 0b010,  // port 1, pin 1
        Blue  = 0b100,  // port 1, pin 2
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


    /// Simple API for sending/receiving data to/from Delcom's visual
    /// indicator USB HID. Relies on libusb for communication. Object
    /// does *not* take ownership of device handle passed to constructor
    /// -- user is responsible for freeing/releasing the device handle.
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
        bool turn_led_on(Color) const;
        bool turn_led_off(Color) const;

        // can only set PWM for one color at a time
        bool set_pwm(Color, std::uint8_t pct) const;

        // port access
        port_data read_ports_and_pins() const;

        /// \returns event-counter value and overflow status
        std::tuple<std::uint32_t, bool> read_and_reset_event_counter() const;

    private:
        bool send_set_report(packet const&) const;
        std::size_t send_get_report(packet&) const;
    };

} // namespace delcom

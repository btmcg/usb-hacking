#pragma once

#include "protocol.hpp"
#include "usb_hid.hpp"
#include <fmt/format.h>
#include <libusb.h>
#include <cstddef> // std::size_t
#include <cstdint>
#include <mutex>
#include <thread>
#include <tuple>
#include <vector>


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

    // clang-format off
    enum class Color : std::uint8_t
    {
        Green = 0b001,  // port 1, pin 0
        Red   = 0b010,  // port 1, pin 1
        Blue  = 0b100,  // port 1, pin 2
    };
    // clang-format on

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
    /// indicator USB HID. Relies on libusb for communication.
    /// \ref vendor id = 0x0fc5
    /// \ref product id = 0xb080
    /// \ref family type id = 2
    /// \ref https://www.delcomproducts.com/productdetails.asp?PartNumber=900000
    class vi_hid
    {
    private:
        libusb_context* ctx_ = nullptr;
        libusb_device_handle* dev_ = nullptr;
        std::uint16_t vendor_id_ = 0;
        std::uint16_t product_id_ = 0;
        std::uint16_t interface_ = 0;
        std::size_t initial_pwm_ = 50; ///< half (50%)
        std::mutex threads_lock_;
        std::vector<std::thread> threads_;

    public:
        vi_hid(std::uint16_t vendor_id, std::uint16_t product_id, bool debug = false);
        ~vi_hid() noexcept;

        std::uint16_t vendor_id() const noexcept;
        std::uint16_t product_id() const noexcept;
        firmware_info read_firmware_info() const;

        /// A duration of 0 turns the light on until \c turn_led_off is
        /// called. Returns immediately, regardless of duration.
        bool turn_led_on(Color color, std::uint64_t duration_msecs = 0);
        bool turn_led_off(Color) const;
        bool turn_off_leds_on_button_press(bool enable) const;

        /// Set led intensity, where 0 <= pct <= 100. Note that a pct of
        /// 0 means off.
        bool set_led_intensity(Color, std::uint8_t pct) const;

        port_data read_port_data() const;

        /// \returns event-counter value and overflow status
        std::tuple<std::uint32_t, bool> read_and_reset_event_counter() const;

    private:
        bool initialize_device() const;
        bool led(bool enable, Color) const;
        bool set_pwm(Color, std::uint8_t) const;
        std::size_t send_get_report(packet&) const;
        bool send_set_report(packet const&) const;
    };

} // namespace delcom

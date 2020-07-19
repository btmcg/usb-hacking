#include "delcom.hpp"
#include "util/assert.hpp"
#include <fmt/format.h>
#include <libusb.h>


namespace delcom {

    std::string
    to_str(send_cmd const& msg)
    {
        return fmt::format("    major_cmd  = {0:#010b} {0:#03d} {0:#04x} {1:s}\n"
                           "    minor_cmd  = {2:#010b} {2:#03d} {2:#04x} {3:s}\n"
                           "    lsb        = {4:#010b} {4:#03d} {4:#04x}\n"
                           "    msb        = {5:#010b} {5:#03d} {5:#04x}\n"
                           "    data_hid0  = {6:#010b} {6:#03d} {6:#04x}\n"
                           "    data_hid1  = {7:#010b} {7:#03d} {7:#04x}\n"
                           "    data_hid2  = {8:#010b} {8:#03d} {8:#04x}\n"
                           "    data_hid3  = {9:#010b} {9:#03d} {9:#04x}",
                msg.cmd, to_str(msg.cmd), msg.write_cmd, to_str(msg.write_cmd), msg.lsb, msg.msb,
                msg.data_hid[0], msg.data_hid[1], msg.data_hid[2], msg.data_hid[3]);
    }

    std::string
    to_str(recv_cmd const& msg)
    {
        return fmt::format(
                "    major_cmd  = {0:#010b} {0:#03d} {0:#04x} {1:s}", msg.cmd, to_str(msg.cmd));
    }

    vi_hid::vi_hid(libusb_device_handle* handle)
            : dev_(handle)
    {
        DEBUG_ASSERT(dev_ != nullptr);

        if (::libusb_kernel_driver_active(dev_, interface_) == 1) {
            if (int e = ::libusb_detach_kernel_driver(dev_, interface_); e != LIBUSB_SUCCESS) {
                throw std::runtime_error(fmt::format("{}: libusb_detach_kernel_driver failure ({})",
                        __builtin_FUNCTION(), ::libusb_strerror(static_cast<libusb_error>(e))));
            }
        }

        // if (int e = ::libusb_set_auto_detach_kernel_driver(dev_, /*enable=*/1);
        //         e != LIBUSB_SUCCESS) {
        //     throw std::runtime_error(
        //             fmt::format("libusb: set_auto_detach_kernel_driver failure ({})",
        //                     ::libusb_strerror(static_cast<libusb_error>(e))));
        // }

        if (int e = ::libusb_claim_interface(dev_, interface_); e != LIBUSB_SUCCESS) {
            throw std::runtime_error(fmt::format("{}: libusb_claim_interface failure ({})",
                    __builtin_FUNCTION(), ::libusb_strerror(static_cast<libusb_error>(e))));
        }

        // set up device by first turning off all leds and setting the
        // PWM to full (100) for each

        bool rv = turn_led_off(Color::Red | Color::Green | Color::Blue);
        DEBUG_ASSERT(rv);

        // set PWM to 100 for every led
        rv = set_pwm(Color::Green, 100) && set_pwm(Color::Red, 100) && set_pwm(Color::Blue, 100);
        DEBUG_ASSERT(rv);
    }

    vi_hid::~vi_hid() noexcept
    {
        if (dev_ == nullptr)
            return;

        if (int e = ::libusb_release_interface(dev_, interface_); e != LIBUSB_SUCCESS) {
            std::fprintf(stderr, "libusb: release_interface failure (%s)\n",
                    ::libusb_strerror(static_cast<libusb_error>(e)));
        }
    }

    firmware_info
    vi_hid::read_firmware_info() const
    {
        packet msg = {0};
        msg.recv.cmd = Command::ReadFirmware;

        try {
            send_get_report(msg);
        } catch (std::exception const& e) {
            throw std::runtime_error(fmt::format(
                    "{}: send_get_report failure ({})", __builtin_FUNCTION(), e.what()));
        }

        auto fi_ptr = reinterpret_cast<fw_info const*>(msg.data);
        firmware_info info;
        info.serial_number = fi_ptr->serial_number;
        info.version = fi_ptr->version;
        info.year = 2000 + fi_ptr->year;
        info.month = fi_ptr->month;
        info.day = fi_ptr->date;

        return info;
    }

    bool
    vi_hid::turn_led_on(Color color) const
    {
        // to turn an led on, we need to "reset" that color's pin (set
        // it to 0)

        packet msg = {0};
        msg.send.cmd = Command::Write8Bytes;
        msg.send.write_cmd = WriteCommand::SetOrResetPort1;
        msg.send.lsb = static_cast<std::uint8_t>(color); // pins to reset
        msg.send.msb = 0; // pins to set

        try {
            return send_set_report(msg);
        } catch (std::exception const& e) {
            throw std::runtime_error(fmt::format(
                    "{}: send_set_report failure ({})", __builtin_FUNCTION(), e.what()));
        }
    }

    bool
    vi_hid::turn_led_off(Color color) const
    {
        // to turn an led off, we need to "set" that color's pin (set it
        // to 1)

        packet msg = {0};
        msg.send.cmd = Command::Write8Bytes;
        msg.send.write_cmd = WriteCommand::SetOrResetPort1;
        msg.send.lsb = 0; // pins to reset
        msg.send.msb = static_cast<std::uint8_t>(color); // pins to set

        try {
            return send_set_report(msg);
        } catch (std::exception const& e) {
            throw std::runtime_error(fmt::format(
                    "{}: send_set_report failure ({})", __builtin_FUNCTION(), e.what()));
        }
    }

    port_data
    vi_hid::read_ports_and_pins() const
    {
        packet msg = {0};
        msg.recv.cmd = Command::ReadPort0and1;

        try {
            if (send_get_report(msg) != sizeof(msg))
                return port_data();
        } catch (std::exception const& e) {
            throw std::runtime_error(fmt::format(
                    "{}: send_get_report failure ({})", __builtin_FUNCTION(), e.what()));
        }

        port_data pd;
        pd.port0 = msg.data[0];
        pd.port1 = msg.data[1];
        pd.clock_status = msg.data[2];
        pd.port2 = msg.data[3];
        return pd;
    }

    std::tuple<std::uint32_t, bool>
    vi_hid::read_and_reset_event_counter() const
    {
        packet msg = {0};
        msg.recv.cmd = Command::ReadEventCounter;

        try {
            send_get_report(msg);
        } catch (std::exception const& e) {
            throw std::runtime_error(fmt::format(
                    "{}: send_get_report failure ({})", __builtin_FUNCTION(), e.what()));
        }

        auto info = reinterpret_cast<event_counter_info const*>(msg.data);

        return {info->counter_value, (info->overflow_status == 0xff) ? true : false};
    }

    bool
    vi_hid::set_pwm(Color color, std::uint8_t pct) const
    {
        packet msg = {0};
        msg.send.cmd = Command::Write8Bytes;
        msg.send.write_cmd = WriteCommand::SetPWM;
        // clang-format off
        switch (color)
        {
            case Color::Green:  msg.send.lsb = 0; break;
            case Color::Red:    msg.send.lsb = 1; break;
            case Color::Blue:   msg.send.lsb = 2; break;
        }
        // clang-format on
        msg.send.msb = pct;

        try {
            return send_set_report(msg);
        } catch (std::exception const& e) {
            throw std::runtime_error(fmt::format(
                    "{}: send_set_report failure ({})", __builtin_FUNCTION(), e.what()));
        }
    }

    // private
    /**********************************************************************/

    bool
    vi_hid::send_set_report(packet const& msg) const
    {
        // USB HID definition section 7.2 Class-Specific Requests
        // Request type
        //  Bits 0:4 determine recipient, see \ref libusb_request_recipient.
        //  Bits 5:6 determine type, see \ref libusb_request_type.
        //  Bit 7 determines data transfer direction, see \ref libusb_endpoint_direction.
        std::uint16_t const request_type = static_cast<std::uint8_t>(LIBUSB_RECIPIENT_INTERFACE)
                | static_cast<std::uint8_t>(LIBUSB_REQUEST_TYPE_CLASS)
                | static_cast<std::uint8_t>(LIBUSB_ENDPOINT_OUT);
        std::uint8_t const request = static_cast<std::uint8_t>(usb::hid::ClassRequest::SetReport);
        std::uint16_t const value = static_cast<std::uint8_t>(usb::hid::ReportType::Feature) << 8;
        std::uint8_t const index = interface_;

        // fmt::print("send_set_report [\n"
        //            "  request_type = {0:#010b} {0:#03d} {0:#04x}\n"
        //            "  request      = {1:#010b} {1:#03d} {1:#04x}\n"
        //            "  value        = {2:#018b} {2:#05d} {2:#06x}\n"
        //            "  index        = {3:#010b} {3:#03d} {3:#04x}\n"
        //            "  report data [\n"
        //            "{4:s}\n"
        //            "  ]\n"
        //            "]\n",
        //         request_type, request, value, index, to_str(msg.send));

        int nbytes = ::libusb_control_transfer(dev_, request_type, request, value, index,
                const_cast<std::uint8_t*>(msg.data), sizeof(msg), /*timeout_millis=*/0);
        if (nbytes != sizeof(msg)) {
            throw std::runtime_error(fmt::format("{}: libusb_control_transfer failure ({})",
                    __builtin_FUNCTION(), ::libusb_strerror(static_cast<libusb_error>(nbytes))));
        }

        return true;
    }

    std::size_t
    vi_hid::send_get_report(packet& msg) const
    {
        // USB HID definition section 7.2 Class-Specific Requests
        // Request type
        //  Bits 0:4 determine recipient, see \ref libusb_request_recipient.
        //  Bits 5:6 determine type, see \ref libusb_request_type.
        //  Bit 7 determines data transfer direction, see \ref libusb_endpoint_direction.
        std::uint16_t const request_type = static_cast<std::uint8_t>(LIBUSB_RECIPIENT_INTERFACE)
                | static_cast<std::uint8_t>(LIBUSB_REQUEST_TYPE_CLASS)
                | static_cast<std::uint8_t>(LIBUSB_ENDPOINT_IN);
        std::uint8_t const request = static_cast<std::uint8_t>(usb::hid::ClassRequest::GetReport);
        std::uint16_t const value
                = (static_cast<std::uint8_t>(usb::hid::ReportType::Feature) << 8) | msg.data[0];
        std::uint8_t const index = interface_;

        // fmt::print("send_get_report [\n"
        //            "  request_type = {0:#010b} {0:#03d} {0:#04x}\n"
        //            "  request      = {1:#010b} {1:#03d} {1:#04x}\n"
        //            "  value        = {2:#018b} {2:#05d} {2:#06x}\n"
        //            "  index        = {3:#010b} {3:#03d} {3:#04x}\n"
        //            "  report data [\n"
        //            "{4:s}\n"
        //            "  ]\n"
        //            "]\n",
        //         request_type, request, value, index, to_str(msg.recv));

        int nbytes = ::libusb_control_transfer(dev_, request_type, request, value, index, msg.data,
                sizeof(msg), /*timeout_millis=*/0);
        if (nbytes != sizeof(msg)) {
            throw std::runtime_error(fmt::format("{}: libusb_control_transfer failure ({})",
                    __builtin_FUNCTION(), ::libusb_strerror(static_cast<libusb_error>(nbytes))));
        }

        return static_cast<std::size_t>(nbytes);
    }

} // namespace delcom

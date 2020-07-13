#include "delcom.hpp"
#include "delcom_protocol.hpp"
#include "util/assert.hpp"
#include <fmt/format.h>
#include <libusb.h>


namespace delcom {

    vi_hid::vi_hid(libusb_device_handle* handle)
            : dev_(handle)
    {
        DEBUG_ASSERT(dev_ != nullptr);

        if (int e = ::libusb_set_auto_detach_kernel_driver(dev_, /*enable=*/1);
                e != LIBUSB_SUCCESS) {
            throw std::runtime_error(
                    fmt::format("libusb: set_auto_detach_kernel_driver failure ({})",
                            ::libusb_strerror(static_cast<libusb_error>(e))));
        }

        if (int e = ::libusb_claim_interface(dev_, interface_); e != LIBUSB_SUCCESS) {
            throw std::runtime_error(fmt::format("libusb: claim_interface failure ({})",
                    ::libusb_strerror(static_cast<libusb_error>(e))));
        }
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
        packet msg;
        msg.recv.cmd = Command::ReadFirmware;

        try {
            ctrl_transfer(usb::hid::ClassRequest::GetReport, msg);
        } catch (std::exception const& e) {
            throw std::runtime_error(
                    fmt::format("{}: ctrl transfer failure ({})", __builtin_FUNCTION(), e.what()));
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

    port_data
    vi_hid::read_port_data() const
    {
        packet msg;
        msg.recv.cmd = Command::ReadPort0and1;

        try {
            ctrl_transfer(usb::hid::ClassRequest::GetReport, msg);
        } catch (std::exception const& e) {
            throw std::runtime_error(
                    fmt::format("{}: ctrl transfer failure ({})", __builtin_FUNCTION(), e.what()));
        }

        port_data pd;
        pd.port0 = msg.data[0];
        pd.port1 = msg.data[1];
        pd.clock_enabled = (msg.data[2] != 0);
        pd.port2 = msg.data[3];
        return pd;
    }

    void
    vi_hid::flash_led(Color color) const
    {
        try {
            power_led(color, flash_duration_);
        } catch (std::exception const& e) {
            throw std::runtime_error(
                    fmt::format("{}: power_led failure ({})", __builtin_FUNCTION(), e.what()));
        }
    }

    std::tuple<std::uint32_t, bool>
    vi_hid::read_and_reset_event_counter() const
    {
        packet msg;
        msg.recv.cmd = Command::ReadEventCounter;

        try {
            ctrl_transfer(usb::hid::ClassRequest::GetReport, msg);
        } catch (std::exception const& e) {
            throw std::runtime_error(
                    fmt::format("{}: ctrl transfer failure ({})", __builtin_FUNCTION(), e.what()));
        }

        auto info = reinterpret_cast<event_counter_info const*>(msg.data);

        return {info->counter_value, (info->overflow_status == 0xff) ? true : false};
    }

    void
    vi_hid::set_pwm(Color color, std::uint8_t pct) const
    {
        packet msg;
        msg.send.cmd = Command::Write8Bytes;
        msg.send.write_cmd = WriteCommand::SetPWM;
        msg.send.lsb = static_cast<std::uint8_t>(color);
        msg.send.msb = pct;

        try {
            ctrl_transfer(usb::hid::ClassRequest::SetReport, msg);
        } catch (std::exception const& e) {
            throw std::runtime_error(
                    fmt::format("{}: ctrl transfer failure ({})", __builtin_FUNCTION(), e.what()));
        }
    }

    void
    vi_hid::set_duty_cycle(Color color, std::uint8_t duty_on, std::uint8_t duty_off) const
    {
        packet msg;
        msg.send.cmd = Command::Write8Bytes;
        switch (color) {
            case Color::Green:
                msg.send.write_cmd = WriteCommand::SetDutyCyclePort1Pin0;
                break;
            case Color::Red:
                msg.send.write_cmd = WriteCommand::SetDutyCyclePort1Pin1;
                break;
            case Color::Blue:
                msg.send.write_cmd = WriteCommand::SetDutyCyclePort1Pin2;
                break;
            case Color::None:
                break;
        }
        msg.send.lsb = duty_on;
        msg.send.msb = duty_off;

        try {
            ctrl_transfer(usb::hid::ClassRequest::SetReport, msg);
        } catch (std::exception const& e) {
            throw std::runtime_error(
                    fmt::format("{}: ctrl transfer failure ({})", __builtin_FUNCTION(), e.what()));
        }
    }

    void
    vi_hid::enable_clock(Color color) const
    {
        packet msg;
        msg.send.cmd = Command::Write8Bytes;
        msg.send.write_cmd = WriteCommand::ToggleClockGenPort1;
        msg.send.lsb = 0;
        msg.send.msb = static_cast<std::uint8_t>(color);

        try {
            ctrl_transfer(usb::hid::ClassRequest::SetReport, msg);
        } catch (std::exception const& e) {
            throw std::runtime_error(
                    fmt::format("{}: ctrl transfer failure ({})", __builtin_FUNCTION(), e.what()));
        }
    }

    void
    vi_hid::disable_clock(Color color) const
    {
        packet msg;
        msg.send.cmd = Command::Write8Bytes;
        msg.send.write_cmd = WriteCommand::ToggleClockGenPort1;
        msg.send.lsb = static_cast<std::uint8_t>(color);
        msg.send.msb = 0;

        try {
            ctrl_transfer(usb::hid::ClassRequest::SetReport, msg);
        } catch (std::exception const& e) {
            throw std::runtime_error(
                    fmt::format("{}: ctrl transfer failure ({})", __builtin_FUNCTION(), e.what()));
        }
    }

    // private
    /**********************************************************************/

    void
    vi_hid::power_led(Color color, std::size_t duration) const
    {
        packet msg;
        msg.send.cmd = Command::Write8Bytes;
        msg.send.write_cmd = WriteCommand::Port1;
        msg.send.lsb = static_cast<std::uint8_t>(color);

        try {
            for (decltype(duration) i = 0; i < duration; ++i) {
                ctrl_transfer(usb::hid::ClassRequest::SetReport, msg);
            }
        } catch (std::exception const& e) {
            throw std::runtime_error(
                    fmt::format("{}: ctrl transfer failure ({})", __builtin_FUNCTION(), e.what()));
        }
    }

    void
    vi_hid::ctrl_transfer(usb::hid::ClassRequest request, packet& rpt) const
    {
        // USB HID definition section 7.2 Class-Specific Requests
        // Request type
        //  Bits 0:4 determine recipient, see \ref libusb_request_recipient.
        //  Bits 5:6 determine type, see \ref libusb_request_type.
        //  Bit 7 determines data transfer direction, see \ref libusb_endpoint_direction.
        std::uint16_t bm_request_type = static_cast<std::uint8_t>(LIBUSB_RECIPIENT_INTERFACE)
                | static_cast<std::uint8_t>(LIBUSB_REQUEST_TYPE_CLASS);

        // high byte of w_value is always the ReportType
        std::uint16_t w_value = static_cast<std::uint8_t>(usb::hid::ReportType::Feature) << 8;

        if (request == usb::hid::ClassRequest::GetReport) {
            // Generate a HID Get_Report Request
            bm_request_type |= static_cast<std::uint8_t>(LIBUSB_ENDPOINT_IN);

            // for gets, we set the low byte of w_value to the "cmd"
            w_value |= rpt.data[0];
        } else {
            // Generate a HID Set_Report Request
            bm_request_type |= static_cast<std::uint8_t>(LIBUSB_ENDPOINT_OUT);
        }

        if (int rv = ::libusb_control_transfer(dev_, bm_request_type,
                    static_cast<std::uint8_t>(request), w_value, interface_, rpt.data, sizeof(rpt),
                    ctrl_timeout_msec_);
                rv < 0) {
            throw std::runtime_error(fmt::format("{}: libusb_control_transfer failure ({})",
                    __builtin_FUNCTION(), ::libusb_strerror(static_cast<libusb_error>(rv))));
        }
    }

} // namespace delcom

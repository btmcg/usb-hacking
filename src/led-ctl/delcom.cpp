#include "delcom.hpp"
#include "util/assert.hpp"
#include <fmt/format.h>


namespace delcom {

    namespace { // unnamed

        libusb_device_handle*
        open_device(libusb_context* const ctx, std::uint16_t vid, std::uint16_t pid)
        {
            libusb_device** devices = nullptr;
            ssize_t num_devs = ::libusb_get_device_list(ctx, &devices);
            if (num_devs < 0) {
                fmt::print(stderr, "libusb_get_device_list failure ({})\n",
                        ::libusb_strerror(static_cast<libusb_error>(num_devs)));
                ::libusb_free_device_list(devices, 1);
                return nullptr;
            }

            libusb_device* dev = nullptr;
            for (ssize_t i = 0; i < num_devs; ++i) {
                libusb_device_descriptor dd;
                if (int rv = ::libusb_get_device_descriptor(devices[i], &dd); rv != 0) {
                    fmt::print(stderr, "libusb_get_device_descriptor failure ({})\n",
                            ::libusb_strerror(static_cast<libusb_error>(num_devs)));
                    ::libusb_free_device_list(devices, 1);
                    return nullptr;
                }

                if (dd.idVendor == vid && dd.idProduct == pid) {
                    dev = devices[i];
                    break;
                }
            }

            libusb_device_handle* dev_handle = nullptr;
            if (dev != nullptr) {
                if (int rv = ::libusb_open(dev, &dev_handle); rv != 0) {
                    fmt::print(stderr, "libusb_open failure ({})\n",
                            ::libusb_strerror(static_cast<libusb_error>(rv)));
                    dev = nullptr;
                    dev_handle = nullptr;
                }
            }

            // decrements all device counts by 1
            ::libusb_free_device_list(devices, 1);
            return dev_handle;
        }

    } // namespace


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

    vi_hid::vi_hid(std::uint16_t vid, std::uint16_t pid, bool debug)
            : vendor_id_(vid)
            , product_id_(pid)
    {
        if (int e = ::libusb_init(&ctx_); e != LIBUSB_SUCCESS) {
            throw std::runtime_error(fmt::format("{}: libusb_init failure ({})",
                    __builtin_FUNCTION(), ::libusb_strerror(static_cast<libusb_error>(e))));
        }

        if (debug) {
            if (int e = ::libusb_set_option(ctx_, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_DEBUG);
                    e != LIBUSB_SUCCESS) {
                ::libusb_exit(ctx_);
                throw std::runtime_error(fmt::format("{}: libusb_set_option failure ({})",
                        __builtin_FUNCTION(), ::libusb_strerror(static_cast<libusb_error>(e))));
            }
        }

        if (dev_ = open_device(ctx_, vendor_id_, product_id_); dev_ == nullptr) {
            ::libusb_exit(ctx_);
            throw std::runtime_error(fmt::format("{}: failed to open device {:#06x}:{:#06x}",
                    __builtin_FUNCTION(), vendor_id_, product_id_));
        }

        // if (::libusb_kernel_driver_active(dev_, interface_) == 1) {
        //     if (int e = ::libusb_detach_kernel_driver(dev_, interface_); e != LIBUSB_SUCCESS) {
        //         ::libusb_close(dev_);
        //         ::libusb_exit(ctx_);
        //         throw std::runtime_error(fmt::format("{}: libusb_detach_kernel_driver failure
        //         ({})",
        //                 __builtin_FUNCTION(), ::libusb_strerror(static_cast<libusb_error>(e))));
        //     }
        // }

        if (int e = ::libusb_set_auto_detach_kernel_driver(dev_, /*enable=*/1);
                e != LIBUSB_SUCCESS) {
            ::libusb_close(dev_);
            ::libusb_exit(ctx_);
            throw std::runtime_error(
                    fmt::format("{}: libusb_set_auto_detach_kernel_driver failure ({})",
                            __builtin_FUNCTION(), ::libusb_strerror(static_cast<libusb_error>(e))));
        }

        if (int e = ::libusb_claim_interface(dev_, interface_); e != LIBUSB_SUCCESS) {
            ::libusb_close(dev_);
            ::libusb_exit(ctx_);
            throw std::runtime_error(fmt::format("{}: libusb_claim_interface failure ({})",
                    __builtin_FUNCTION(), ::libusb_strerror(static_cast<libusb_error>(e))));
        }

        if (!initialize_device()) {
            ::libusb_close(dev_);
            ::libusb_exit(ctx_);
            throw std::runtime_error(
                    fmt::format("{}: failed to initialized device", __builtin_FUNCTION()));
        }
    }

    vi_hid::~vi_hid() noexcept
    {
        {
            std::lock_guard l(threads_lock_);
            for (auto& t : threads_)
                t.join();
        }

        if (int e = ::libusb_release_interface(dev_, interface_); e != LIBUSB_SUCCESS) {
            std::fprintf(stderr, "libusb: release_interface failure (%s)\n",
                    ::libusb_strerror(static_cast<libusb_error>(e)));
        }

        ::libusb_close(dev_);
        ::libusb_exit(ctx_);
    }

    std::uint16_t
    vi_hid::vendor_id() const noexcept
    {
        return vendor_id_;
    }

    std::uint16_t
    vi_hid::product_id() const noexcept
    {
        return product_id_;
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
    vi_hid::turn_led_on(Color color, std::uint64_t duration_msecs)
    {
        bool success = led(true, color);

        if (success && duration_msecs != 0) {
            std::lock_guard l(threads_lock_);

            // clean up any joinable threads
            std::erase_if(threads_, [](auto& t) { return t.joinable(); });

            threads_.emplace_back([this, color, duration_msecs]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(duration_msecs));
                led(false, color);
            });
        }

        return success;
    }

    bool
    vi_hid::turn_led_off(Color color) const
    {
        return led(false, color);
    }

    bool
    vi_hid::set_led_intensity(Color color, std::uint8_t pct) const
    {
        // 'pct' must be 0 <= pct <= 100
        if (pct > 100) {
            fmt::print(stderr, "{}: invalid pct ({}) provided; constraints are 0 <= pct <= 100\n",
                    __builtin_FUNCTION(), pct);
            return false;
        }

        return set_pwm(color, pct);
    }

    port_data
    vi_hid::read_port_data() const
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


    // private
    /**********************************************************************/

    bool
    vi_hid::initialize_device() const
    {
        // Set up device by first turning off all leds and setting the
        // PWM to the preferred initial value.

        if (!turn_led_off(Color::Red | Color::Green | Color::Blue)) {
            fmt::print(stderr, "{}: turn_led_off failure\n", __builtin_FUNCTION());
            return false;
        }

        if (!set_pwm(Color::Red | Color::Green | Color::Blue, initial_pwm_)) {
            fmt::print(stderr, "{}: set_pwm failure\n", __builtin_FUNCTION());
            return false;
        }

        return true;
    }

    bool
    vi_hid::led(bool enable, Color color) const
    {
        // to turn an led on, we need to "reset" that color's pin (set
        // it to 0), and conversely, to turn an led off, "set" that pin
        // (set it to 1)
        //
        // lsb are the pins that will be reset
        // msb are the pins that will be set
        // (resetting takes precedence)

        packet msg = {0};
        msg.send.cmd = Command::Write8Bytes;
        msg.send.write_cmd = WriteCommand::SetOrResetPort1;
        if (enable)
            msg.send.lsb = static_cast<std::uint8_t>(color); // pins to reset
        else
            msg.send.msb = static_cast<std::uint8_t>(color); // pins to set

        try {
            return send_set_report(msg);
        } catch (std::exception const& e) {
            throw std::runtime_error(fmt::format(
                    "{}: send_set_report failure ({})", __builtin_FUNCTION(), e.what()));
        }
    }

    bool
    vi_hid::set_pwm(Color color, std::uint8_t pct) const
    {
        // The SetPWM command sets the pulse-width modulation for a
        // particular LED pin. The command takes two integers: a decimal
        // value that refers to an LED pin (0, 1, 2), and a percentage
        // value from 0 to 100. Since each command accepts a single pin
        // to be changed, we need to send one message per pin/color.
        packet msg = {0};
        msg.send.cmd = Command::Write8Bytes;
        msg.send.write_cmd = WriteCommand::SetPWM;
        msg.send.lsb = 0; // will be set to pin specified by 'color'
        msg.send.msb = pct;

        bool p0 = true, p1 = true, p2 = true;
        try {
            if ((color & Color::Green) == Color::Green) {
                msg.send.lsb = 0;
                p0 = send_set_report(msg);
                if (!p0) {
                    fmt::print(stderr, "{}: send_set_report failure: failed to set PWM for pin 0\n",
                            __builtin_FUNCTION());
                }
            }
            if ((color & Color::Red) == Color::Red) {
                msg.send.lsb = 1;
                p1 = send_set_report(msg);
                if (!p1) {
                    fmt::print(stderr, "{}: send_set_report failure: failed to set PWM for pin 1\n",
                            __builtin_FUNCTION());
                }
            }
            if ((color & Color::Blue) == Color::Blue) {
                msg.send.lsb = 2;
                p2 = send_set_report(msg);
                if (!p2) {
                    fmt::print(stderr, "{}: send_set_report failure: failed to set PWM for pin 2\n",
                            __builtin_FUNCTION());
                }
            }
        } catch (std::exception const& e) {
            throw std::runtime_error(fmt::format(
                    "{}: send_set_report failure ({})", __builtin_FUNCTION(), e.what()));
        }

        return p0 && p1 && p2;
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

} // namespace delcom

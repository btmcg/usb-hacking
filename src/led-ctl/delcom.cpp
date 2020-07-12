#include "delcom.hpp"
#include "usb_hid.hpp"
#include "util/assert.hpp"
#include "util/compiler.hpp"
#include <fmt/format.h>
#include <libusb.h>


namespace delcom {


    // static_assert(sizeof(command_packet.send) == 8);
    // static_assert(sizeof(command_packet.recv) == 8);

    // struct command_packet
    // {
    //     std::uint8_t major_cmd = 0;
    //     std::uint8_t minor_cmd = 0;
    //     std::uint8_t lsb = 0;
    //     std::uint8_t msb = 0;
    //     std::uint8_t data[4];
    // } PACKED;

    struct fw_info
    {
        std::uint32_t serial_number = 0;
        std::uint8_t version = 0;
        std::uint8_t date = 0;
        std::uint8_t month = 0;
        std::uint8_t year = 0;

    } PACKED;
    static_assert(sizeof(fw_info) == 8);

    enum class MajorCommand : std::uint8_t
    {
        // clang-format off
        ReadFirmware    = 10,
        ReadPort0and1   = 100,
        Write           = 101,
        // clang-format on
    };

    enum class MinorCommand : std::uint8_t
    {
        Port0 = 2,
    };

    struct send_cmd
    {
        MajorCommand major_cmd;
        MinorCommand minor_cmd;
        std::uint8_t lsb;
        std::uint8_t msb;
    } PACKED;

    struct recv_cmd
    {
        MajorCommand cmd;
    } PACKED;

    union packet
    {
        std::uint8_t data[8] = {0};
        send_cmd send;
        recv_cmd recv;
    } PACKED;
    static_assert(sizeof(packet) == 8);


    /**********************************************************************/

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
    vi_hid::get_firmware_info() const
    {
        // Generate a HID Get_Report Request

        // Request type
        //  Bits 0:4 determine recipient, see \ref libusb_request_recipient.
        //  Bits 5:6 determine type, see \ref libusb_request_type.
        //  Bit 7 determines data transfer direction, see \ref libusb_endpoint_direction.
        std::uint8_t const req_type = (static_cast<std::uint8_t>(LIBUSB_REQUEST_TYPE_CLASS)
                | static_cast<std::uint8_t>(LIBUSB_RECIPIENT_INTERFACE)
                | static_cast<std::uint8_t>(LIBUSB_ENDPOINT_IN));
        std::uint8_t const req = static_cast<std::uint8_t>(usb::hid::ClassRequest::GetReport);
        std::uint8_t const report_type = static_cast<std::uint8_t>(usb::hid::ReportType::Feature);
        std::uint8_t const report_id = static_cast<std::uint8_t>(MajorCommand::ReadFirmware);
        std::uint16_t const value = (report_type << 8) | report_id;

        packet msg;
        msg.recv.cmd = MajorCommand::ReadFirmware;

        if (int nbytes = ::libusb_control_transfer(dev_, req_type, req, value, interface_, msg.data,
                    sizeof(msg), ctrl_timeout_msec_);
                nbytes <= 0) {
            throw std::runtime_error(fmt::format("libusb_control_transfer failure ({})\n",
                    ::libusb_strerror(static_cast<libusb_error>(nbytes))));
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

    void
    vi_hid::flash_led(Color color) const
    {
        // Generate a HID Set_Report Request

        // Request type
        //  Bits 0:4 determine recipient, see \ref libusb_request_recipient.
        //  Bits 5:6 determine type, see \ref libusb_request_type.
        //  Bit 7 determines data transfer direction, see \ref libusb_endpoint_direction.
        std::uint8_t const req_type = (static_cast<std::uint8_t>(LIBUSB_REQUEST_TYPE_CLASS)
                | static_cast<std::uint8_t>(LIBUSB_RECIPIENT_INTERFACE)
                | static_cast<std::uint8_t>(LIBUSB_ENDPOINT_OUT));
        std::uint8_t const req = static_cast<std::uint8_t>(usb::hid::ClassRequest::SetReport);
        std::uint16_t const value = static_cast<std::uint8_t>(usb::hid::ReportType::Feature);

        packet msg;
        msg.send.major_cmd = MajorCommand::Write;
        msg.send.minor_cmd = MinorCommand::Port0;
        msg.send.lsb = static_cast<std::uint8_t>(color);

        for (int i = 0; i < flash_duration_; ++i) {
            if (int rv = ::libusb_control_transfer(dev_, req_type, req, value, interface_, msg.data,
                        sizeof(msg), ctrl_timeout_msec_);
                    rv <= LIBUSB_SUCCESS) {
                throw std::runtime_error(fmt::format("libusb_control_transfer failure ({})\n",
                        ::libusb_strerror(static_cast<libusb_error>(rv))));
            }
        }
    }

} // namespace delcom

#pragma once

#include <cstdint>


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
        Blue    = 0x03,
        Red     = 0x05,
        Green   = 0x06,
        None    = 0x07,
        // clang-format on
    };

    /// Delcom's visual indicator USB HID
    /// vendor id = 0x0fc5
    /// product id = 0xb080
    /// family type id = 2
    /// https://www.delcomproducts.com/productdetails.asp?PartNumber=900000
    class vi_hid
    {
    public:
        static constexpr std::uint16_t vendor_id = 0x0fc5;
        static constexpr std::uint16_t product_id = 0xb080;

    private:
        libusb_device_handle* dev_ = nullptr;
        std::uint32_t ctrl_timeout_msec_ = 1000;
        std::uint8_t interface_ = 0;
        int flash_duration_ = 500;

    public:
        explicit vi_hid(libusb_device_handle*);
        ~vi_hid() noexcept;

        firmware_info get_firmware_info() const;
        void flash_led(Color) const;
    };

} // namespace delcom

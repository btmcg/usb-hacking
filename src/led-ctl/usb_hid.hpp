#pragma once

#include <cstdint>


// https://www.usb.org/document-library/device-class-definition-hid-111


namespace usb::hid {
    inline namespace v1_11 {

        /// Section 7.2
        enum class ClassRequest : std::uint8_t
        {
            // clang-format off
            GetReport   = 1,
            GetIdle     = 2,
            GetProtocol = 3,
            SetReport   = 9,
            SetIdle     = 10,
            SetProtocol = 11,
            // clang-format on
        };

        /// Section 7.2.1
        enum class ReportType : std::uint8_t
        {
            // clang-format off
            Input   = 1,
            Output  = 2,
            Feature = 3,
            // clang-format on
        };

    } // namespace v1_11
} // namespace usb::hid

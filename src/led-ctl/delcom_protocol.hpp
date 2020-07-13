#pragma once

#include "util/compiler.hpp"
#include <cstdint>


namespace delcom {

    // confirmed to work with firmware version 58
    inline namespace v58 {

        /// The command to send to the VI HID. This is referred to as
        /// the "major command" in Delcom documentation. The Write
        /// command should be supplemented with an additional
        /// WriteCommand.
        enum class Command : std::uint8_t
        {
            // clang-format off
            ReadEventCounter = 8,
            ReadFirmware     = 10,
            ReadPort0and1    = 100,
            Write            = 101,
            // clang-format on
        };

        /// The write command to send to the VI HID. This is referred to
        /// as the "minor command" in Delcom documentation. These are
        /// only used if the Command provided was Write.
        enum class WriteCommand : std::uint8_t
        {
            /// Write port 0. Writes the LSB data to port 0.
            Port0 = 1,

            /// Write port 1. Writes the LSB data to port 1.
            Port1 = 2,

            /// Write both the port 0 and port 1 values. The LSB data is
            /// written to port 0 and the MSB to port 1.
            Port0andPort1 = 10,

            /// Sets or resets the port 0 pins individually. The LSB
            /// resets the corresponding port pin(s) and the MSB sets
            /// the corresponding port pin(s) on port 0. Resetting the
            /// port pin(s) takes precendence over setting.
            SetOrResetPort0 = 11,

            /// Sets or resets the port 1 pins individually. The LSB resets
            /// the corresponding port pin(s) and the MSB sets the
            /// corresponding port pin(s) on port 1. Resetting the port pin(s)
            /// takes precendence over setting.
            SetOrResetPort1 = 12,

            /// Loads the Clock Generator Global Prescalar value. This value
            /// is passed in the LSB register. Increasing this number
            /// decreases all the clock function frequencies. Prescalar range
            /// is 1 to 255 and the boot up default value is 10. This is the
            /// master prescalar for all the LED pins. Increasing this
            /// number will slow down the flash rate.
            SetClockGen = 19,

            /// Enables or disables the clock generator on port 1. The lower
            /// nibble of the LSB disables the corresponding port pin(s) and
            /// the lower nibble of the MSB enables the corresponding port
            /// pin(s). Disabling the port pin(s) takes precedence over
            /// enabling. This command enables the flash mode on the
            /// appropriate LED pin. Note you must also turn the LED pin on
            /// with one of the port control commands (e.g. 101-2 or
            /// 101-12).
            ToggleClockGenPort1 = 20,

            /// Loads the frequency and duty cycle for port 1 pin 0. The LSB
            /// data sets the high duty cycle and the MSB data sets the low
            /// duty cycle on port 1 pin 0. This command will set the flash
            /// rate and duty cycle on the LED pin.
            SetDutyCyclePort1Pin0 = 21,

            /// Loads the frequency and duty cycle for port 1 pin 1. The LSB
            /// data sets the high duty cycle and the MSB data sets the low
            /// duty cycle on port 1 pin 1. This command will set the flash
            /// rate and duty cycle on the LED pin.
            SetDutyCyclePort1Pin1 = 22,

            /// Loads the frequency and duty cycle for port 1 pin 2. The LSB
            /// data sets the high duty cycle and the MSB data sets the low
            /// duty cycle on port 1 pin 2. This command will set the flash
            /// rate and duty cycle on the LED pin.
            SetDutyCyclePort1Pin2 = 23,

            /// Synchronizes the clock generation. This command synchronizes
            /// all the clock generators to start now plus an initial phase
            /// delay, see below. The lower nibble of the LSB enables this
            /// function on the corresponding pins P1.0 to P1.3. The lower
            /// nibble of the MSB presets the initial value on the
            /// corresponding pins P1.0 to P1.3. Initial phase delay
            /// resolution is in 10ms and is passed in the LSB register.
            /// Initial phase delay registers are cleared after this command
            /// is sent. Therefore the initial phase delay registers must be
            /// set each time this command is called. Use this command to
            /// sync a LED flash pattern.
            SyncClockGen = 25,

            /// Load initial phase delay on port 1 pin 0. Sets the LSB data
            /// as the port1 pin0 initial delay value. To be used with command 25.
            SetInitialPhaseDelayPort1Pin0 = 26,

            /// Load initial phase delay on port 1 pin 1. Sets the LSB data
            /// as the port1 pin0 initial delay value. To be used with command 25.
            SetInitialPhaseDelayPort1Pin1 = 27,

            /// Load initial phase delay on port 1 pin 2. Sets the LSB data
            /// as the port1 pin0 initial delay value. To be used with command 25.
            SetInitialPhaseDelayPort1Pin2 = 28,

            /// Load initial phase delay on port 1 pin 3. Sets the LSB data
            /// as the port1 pin0 initial delay value. To be used with command 25.
            SetInitialPhaseDelayPort1Pin3 = 29,

            /// Load the PWM values. Port pins P1.0 through P1.3 can be
            /// placed in PWM mode by writing the PWM value with this
            /// command. The LSB Data parameter is the port pin number,
            /// range is 0-3. The MSB Data parameter is the PWM value, range
            /// is 0-100. This command is used to set the power or
            /// brightness for the LED pins. Default value is 80 (80%).
            SetPWM = 34,

            /// Enable/Disable Events Counter. This command sets up the
            /// event counter. This command is useful for capturing the
            /// internal switch event on models that have a switch. The
            /// event counter is a more efficient then just polling the port
            /// pins for the event. See the USBIOHID datasheet for more
            /// information. The internal switch is mapped to port 0 pin 0.
            ToggleEventCounter = 38,

            /// Buzzer Control. This command is used to drive a buzzer on
            /// port 0 pin3. See the buzzer control description in the
            /// USBIOHID datasheet for more information.
            BuzzerCtrl = 70,

            /// Auto Clear & Auto Confirm Control. This command
            /// enables/disables the Auto Clear and Auto Confirm feature.
            /// Bit 6 of the DataLSB controls the Auto Clear feature and bit
            /// 7 controls the Auto Confirm feature. A high value enables
            /// the feature and low disables it. When enabled, the auto
            /// clear control will turn off all the LED pins when the
            /// internal switch (P0.0) is pressed. When enabled, auto
            /// confirm control will sound two quick buzzer tones to
            /// indicate that the switch was pressed.
            AutoClearAutoConfirmCtrl = 72,
        };

        /// Describes the structure of a command that is sent as a USB
        /// SetReport.
        struct send_cmd
        {
            Command cmd;
            WriteCommand write_cmd;
            std::uint8_t lsb;
            std::uint8_t msb;
        } PACKED;

        /// Describes the structure of a command that is sent as a USB
        /// GetReport.
        struct recv_cmd
        {
            Command cmd;
        } PACKED;

        /// Packet containing all commands to be sent to the VI HID. Both
        /// Read and Write commands are written via recv_cmd and send_cmd
        /// respectively. Underlying data accessible via data member.
        union packet
        {
            std::uint8_t data[8] = {0};
            recv_cmd recv;
            send_cmd send;
        } PACKED;
        static_assert(sizeof(packet) == 8);

        /// Structure of firmware info received from VI HID.
        struct fw_info
        {
            std::uint32_t serial_number = 0;
            std::uint8_t version = 0;
            std::uint8_t date = 0;
            std::uint8_t month = 0;
            std::uint8_t year = 0;

        } PACKED;
        static_assert(sizeof(fw_info) == 8);

    } // namespace v58
} // namespace delcom

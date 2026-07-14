// Based on SatCom Library by cafe-alpha, Original:
// http://ppcenter.free.fr/satcom/

#pragma once
#include <cstddef> // For size_t
#include <cstdint> // For uintptr_t, size_t, uint8_t, uint32_t
#include <initializer_list>
#include <srl_register.hpp>

/**
 * @brief Namespace for interacting with a USB development cartridge for the
 * Sega Saturn.
 *
 * This provides access to registers for USB communication, SD card access, and
 * other hardware features.
 */
namespace SRL
{
    /** @brief Namespace for Sega Saturn USB development cartridge hardware access.
     */
    namespace DevCart
    {

        /** @brief CS0 area: Flash memory and USB-related registers.
         *
         *  This namespace groups constants and functions for accessing the cartridge's
         * CS0 memory space, which includes flash memory and USB communication registers
         * (likely for a Sega Saturn USB dev cart). Addresses are memory-mapped I/O;
         * accesses should use volatile pointers to prevent optimization issues.
         */
        namespace CS0
        {
            /** @brief Base address of the cartridge in CS0 area.
             *
             *  This is the starting point for flash and USB registers (overlaps with
             * DataCart in srl_cartridge.hpp).
             */
            constexpr static uintptr_t CART_BASE_ADR =
                0x22000000UL; // Base address of the cartridge in CS0 area

            /** @brief Address of the Wasca Prepare counter register.
             */
            constexpr static uintptr_t CART_PCNTR =
                CART_BASE_ADR + 0x1FFFFF0UL; // Wasca Prepare counter.

            /** @brief Address of the Wasca Status register.
             */
            constexpr static uintptr_t CART_STATUS =
                CART_BASE_ADR + 0x1FFFFF2UL; // Wasca Status register.

            /** @brief Address of the Wasca hardware version register major/minor (e.g. 0x050C = v5.12).
             */
            constexpr static uintptr_t CART_HWVER =
                CART_BASE_ADR +
                0x1FFFFF6UL; // wasca hardware version, major and minor 0x050C = v5.12

            /** @brief Address of the Signature register ("wasca " in ASCII: 0x7761 0x7363 0x6120).
             */
            constexpr static uintptr_t CART_SIGNATURE =
                CART_BASE_ADR +
                0x1FFFFFAUL; // Signature: “wasca “ in ASCII (0x7761 0x7363 0x6120)

            /** @brief Base address of the flash memory (1MB region).
             */
            constexpr static uintptr_t FLASH_MEMORY_BASE =
                CART_BASE_ADR + 0x0; // Base address of the flash memory (1MB)

            /** @brief Address of the USB flags register (8-bit Read/Write).
             *
             *  This register holds status flags for USB FIFO operations (RXF, TXE, PWREN).
             */
            constexpr static uintptr_t USB_FLAGS =
                CART_BASE_ADR +
                0x200001UL; // Address of the USB flags register (Read/Write)

            /** @brief Address of the USB FIFO data register (8-bit Read/Write).
             *
             *  Used for sending/receiving bytes over USB.
             */
            constexpr static uintptr_t USB_FIFO =
                CART_BASE_ADR +
                0x100001; // Address of the USB FIFO data register (Read/Write)
            // 0x223x to 0x227x unused  // Reserved/unused address range in hardware

            /**
             * @brief Registers for controlling the SD card on the development cartridge.
             *
             * These registers are mapped in the CS0 memory space.
             */
            namespace SDCardRegisters
            {
                /** @brief Address of the Card Identification Number Register.
                 */
                constexpr static uintptr_t CART_CID =
                    CART_BASE_ADR + 0x1FF0200UL; // Card Identification Number Register

                /** @brief Address of the Card Specific Data Register.
                 */
                constexpr static uintptr_t CART_CSD =
                    CART_BASE_ADR + 0x1FF0210UL; // Card Specific Data Register

                /** @brief Address of the Operation Condition Register.
                 */
                constexpr static uintptr_t CART_OCR =
                    CART_BASE_ADR + 0x1FF0220UL; // Operation Condition Register

                /** @brief Address of the SD Card Status Register.
                 */
                constexpr static uintptr_t CART_SR =
                    CART_BASE_ADR + 0x1FF0224UL; // SD Card Status Register

                /** @brief Address of the Relative Card Address Register.
                 */
                constexpr static uintptr_t CART_RC =
                    CART_BASE_ADR + 0x1FF0228UL; // Relative Card Address Register

                /** @brief Address of the Command Argument Register.
                 */
                constexpr static uintptr_t CART_CMD_ARG =
                    CART_BASE_ADR + 0x1FF022CUL; // Command Argument Register

                /** @brief Address of the Command Register.
                 */
                constexpr static uintptr_t CART_CMD =
                    CART_BASE_ADR + 0x1FF0230UL; // Command Register

                /** @brief Address of the Auxiliary Status Register.
                 */
                constexpr static uintptr_t CART_ASR =
                    CART_BASE_ADR + 0x1FF0234UL; // Auxiliary Status Register

                /** @brief Address of the Response R1 register.
                 */
                constexpr static uintptr_t CART_RR1 =
                    CART_BASE_ADR + 0x1FF0238UL; // Response R1

                /** @brief Address of the wasca Specific SD Control Register.
                 */
                constexpr static uintptr_t CART_WSSCR =
                    CART_BASE_ADR + 0x1FF0FFEUL; // wasca Specific SD Control Register

            } // namespace SDCardRegisters

            /** @brief Maximum length allowed for firmware uploads (matches flash size).
             */
            constexpr static size_t FIRM_MAXLEN =
                1024 * 1024; // Maximum length allowed for firmware (1MB)

            /**
             * @brief Class representing the USB flags register bits.
             *
             * This class provides a type-safe way to manipulate the bits in the USB flags
             * register (RXF, TXE, PWREN). It supports bitwise operations and flag checking.
             *
             * Note: Only bits 0,1,7 are defined; others are ignored/reserved.
             */
            class USBFlags
            {
            public:
                /** @brief Bit positions for the USB flags register.
                 */
                enum : uint8_t
                {
                    /** @brief RXF: Receive FIFO Full (data available to read).
                     */
                    RXF = 1 << 0, // RXF: Receive FIFO Full (data available to read)
                    /** @brief TXE: Transmit FIFO Empty (ready to accept data).
                     */
                    TXE = 1 << 1, // TXE: Transmit FIFO Empty (ready to accept data)
                    /** @brief PWREN: Power Enable (USB power control).
                     */
                    PWREN = 1 << 7 // PWREN: Power Enable (USB power control)
                };

                /** @brief Mask for all defined flags (bits 0,1,7). */
                static constexpr uint8_t ALL_FLAGS =
                    (RXF | TXE | PWREN); // Mask for all defined flags

                /** @brief Inverted mask for all defined flags (for clearing/checking
                 * undefined bits). */
                static constexpr uint8_t NOT_ALL_FLAGS =
                    static_cast<uint8_t>(~ALL_FLAGS); // Mask for not all defined flags

            private:
                uint8_t bits_; // Raw bit storage (8-bit value read/written to hardware)

            public:
                /** @brief Default constructor: Initializes with no flags set. */
                USBFlags() :
                    bits_(0)
                {}

                /** @brief Constructor: Initialize with raw bit value. */
                explicit USBFlags(uint8_t bits) :
                    bits_(bits)
                {}

                /** @brief Constructor: Initialize by OR-ing a list of flag constants.
                 *  @param flags Initializer list of flag enums (e.g., {USBFlags::RXF,
                 * USBFlags::TXE}).
                 */
                USBFlags(std::initializer_list<uint8_t> flags) :
                    bits_(0)
                {
                    for (auto f : flags)
                        bits_ |= f; // Set each provided flag
                }

                /** @brief Conversion to bool: True if any flag is set. */
                explicit operator bool() const { return bits_ != 0; }

                /** @brief Bitwise OR: Combine with another USBFlags. */
                USBFlags operator|(USBFlags other) const
                {
                    return USBFlags(bits_ | other.bits_);
                }

                /** @brief Bitwise OR assignment: Add flags from another. */
                USBFlags &operator|=(USBFlags other)
                {
                    bits_ |= other.bits_;
                    return *this;
                }

                /** @brief Bitwise AND: Keep only common flags. */
                USBFlags operator&(USBFlags other) const
                {
                    return USBFlags(bits_ & other.bits_);
                }

                /** @brief Bitwise AND assignment: Retain common flags. */
                USBFlags &operator&=(USBFlags other)
                {
                    bits_ &= other.bits_;
                    return *this;
                }

                /** @brief Bitwise NOT: Invert all bits (careful: affects undefined bits too).
                 */
                USBFlags operator~() const { return USBFlags(static_cast<uint8_t>(~bits_)); }

                /** @brief Check if a specific flag is set.
                 *  @param flag The flag constant to test (e.g., USBFlags::TXE).
                 *  @return True if set.
                 */
                bool has(uint8_t flag) const { return (bits_ & flag) != 0; }

                /** @brief Get the raw bit value (for writing to hardware). */
                uint8_t bits() const { return bits_; }
            };

            /**
             * @brief Checks if the Transmit FIFO Empty (TXE) flag is set.
             *
             * Reads the USB_FLAGS register and tests the TXE bit. When the TXE bit is set,
             * the transmit FIFO is full and cannot accept new data. The function name
             * `isTXEFull` is accurate in this context, though `TXE` often means "Transmit
             * Empty" in other hardware.
             *
             * @return true If TXE is set (FIFO is full), false otherwise.
             */
            static inline bool isTXEFull()
            {
                return ((*(volatile uint8_t *)(USB_FLAGS)) & USBFlags::TXE) !=
                       0; // Added volatile for MMIO safety
            }

            /**
             * @brief Reads the raw USB_FLAGS register value.
             */
            static inline uint8_t readFlags() { return *(volatile uint8_t *)(USB_FLAGS); }

            /**
             * @brief Waits until the Transmit FIFO is ready (TXE cleared?).
             * This function polls `isTXEFull()` until it returns false, which indicates
             * that the transmit FIFO is no longer full and can accept data.
             *
             * Warning: Infinite loop if hardware never clears—consider adding timeout in
             * production code.
             *
             */
            static inline void waitTXE()
            {
                // Bad design, no timeout! TODO: Add optional timeout parameter or counter
                while (isTXEFull()); // Busy-wait
            }

            /**
             * @brief Waits until the Transmit FIFO is ready, with timeout.
             *
             * Polls `isTXEFull()` until it returns false. If `maxPolls` reaches zero first,
             * the function returns false to signal timeout.
             *
             * @param maxPolls Maximum number of polling iterations while FIFO is full.
             * @return true if FIFO became ready before timeout, false otherwise.
             */
            static inline bool waitTXE(uint32_t maxPolls)
            {
                while (isTXEFull())
                {
                    if (maxPolls == 0)
                    {
                        return false;
                    }
                    --maxPolls;
                }
                return true;
            }

            /**
             * @brief Checks if the Receive FIFO (RXF) is empty.
             *
             * Reads the USB_FLAGS register and checks the RXF bit.
             * The FIFO is considered empty while RXF is set.
             *
             * @return true If FIFO is empty, false otherwise.
             */
            static inline bool isRXFEmpty()
            {
                return ((*(volatile uint8_t *)(USB_FLAGS)) & USBFlags::RXF) !=
                       0; // Added volatile
            }

            /**
             * @brief Waits until data is available in Receive FIFO.
             *
             * This function polls `isRXFEmpty()` until it returns false, indicating data is
             * ready to be read.
             *
             * Warning: Infinite loop possible—add timeout if needed.
             */
            static inline void waitRXF()
            {
                // Bad design, no timeout !
                while (isRXFEmpty()); // Busy-wait
            }

            /**
             * @brief Waits until data is available in Receive FIFO, with timeout.
             *
             * Polls `isRXFEmpty()` until it returns false. If `maxPolls` reaches zero
             * first, the function returns false to signal timeout.
             *
             * @param maxPolls Maximum number of polling iterations while FIFO is empty.
             * @return true if data became available before timeout, false otherwise.
             */
            static inline bool waitRXF(uint32_t maxPolls)
            {
                while (isRXFEmpty())
                {
                    if (maxPolls == 0)
                    {
                        return false;
                    }
                    --maxPolls;
                }
                return true;
            }

            /**
             * @brief Writes a single byte to the USB FIFO.
             *
             * This function waits until the transmit FIFO is not full (`waitTXE()`) and
             * then writes a single byte.
             *
             * @param c Pointer to the byte to write.
             * @return size_t 1 on success.
             */
            static inline size_t write(const uint8_t *c)
            {
                size_t counter = 0;

                waitTXE();
                *(volatile uint8_t *)(USB_FIFO) = *c; // Volatile for MMIO
                ++counter;
                return counter;
            }

            /**
             * @brief Writes a buffer to the USB FIFO.
             *
             * This function writes a buffer of a given size to the USB FIFO by writing one
             * byte at a time, waiting for the FIFO to be ready for each byte.
             *
             * @param c Pointer to the buffer.
             * @param size Number of bytes to write.
             * @return size_t Number of bytes written.
             */
            static inline size_t write(const uint8_t *c, size_t size)
            {
                size_t counter = 0;
                for (size_t i = 0; i < size; i++)
                {
                    counter += write(c + i);
                }
                return counter;
            }

            /**
             * @brief Reads a single byte from the USB FIFO.
             *
             * This function waits until data is available in the receive FIFO (`waitRXF()`)
             * and then reads a single byte.
             *
             * @return uint8_t The byte read.
             */
            static inline uint8_t read()
            {
                waitRXF();
                return *(volatile uint8_t *)(USB_FIFO); // Volatile for MMIO
            }

            /**
             * @brief Checks if the USB device is connected and ready.
             *
             * This function checks the `USB_FLAGS` register. It assumes the device is
             * connected if the reserved bits (those not in `ALL_FLAGS`) are all zero. This
             * is a common way to detect hardware presence on embedded systems.
             *
             * @return true If connected, false otherwise.
             */
            static inline bool isConnected()
            {
                const uint8_t flags = readFlags();
                // SatCom-compatible test: bits 7..2 must be low when FTDI is USB powered.
                return (flags & 0xFCU) == 0;
            }

            /**
             * @brief Returns true when USB dev cart flag register pattern looks valid.
             */
            static inline bool isPortAvailable()
            {
                const uint8_t flags = readFlags();
                // SatCom-compatible availability test: reserved bits 6..2 should stay low.
                return (flags & 0x7CU) == 0;
            }

        } // namespace CS0

        /** @brief CS1 area: CPLD registers.
         *
         *  This namespace groups constants for accessing the CPLD (Complex Programmable
         * Logic Device) registers, which are used to control features like LEDs, the SD
         * card interface, and general-purpose I/O.
         */
        namespace CS1
        {
            /** @brief Base address for CPLD registers in CS1 space. */
            constexpr static uint32_t CPLD_BASE_ADDR =
                0x24000000L; // Base address for CPLD registers (note: L suffix for long)

            /**
             * @brief Enumeration of CPLD register addresses.
             *
             * These are offsets from `CPLD_BASE_ADDR`. The values `0x55` and `0xAA` are
             * likely part of a handshake or initialization sequence. Access to these
             * registers is typically 8-bit or 16-bit; refer to the hardware documentation
             * for specifics.
             */
            enum class Register : uint32_t
            {
                /** @brief Register CPLD_55 (possibly handshake init/write 0x55).
                 */
                CPLD_55 =
                    CPLD_BASE_ADDR + 0x01,
                /** @brief Register CPLD_AA (possibly handshake init/write 0xAA).
                 */
                CPLD_AA =
                    CPLD_BASE_ADDR + 0x03,
                /** @brief Register: CPLD version (read-only).
                 */
                CART_CPLD_VER = CPLD_BASE_ADDR + 0x05,
                /** @brief Register: Beta/ID identifier.
                 */
                CART_BETA_ID = CPLD_BASE_ADDR + 0x07,
                /** @brief Register: General I/O control.
                 */
                CPLD_IO = CPLD_BASE_ADDR + 0x09,
                /** @brief Register: SD input bits.
                 */
                SDIN_BITS = CPLD_BASE_ADDR + 0x0B,
                /** @brief Register: LED settings (bitfield for colors/modes).
                 */
                LED_SETTING = CPLD_BASE_ADDR +
                              0x0D,
                /** @brief Register: SD clock configuration.
                 */
                SD_CLK_SET = CPLD_BASE_ADDR + 0x0F,
                /** @brief Register: Stdout bit (debug/output).
                 */
                REG_STDOUT_BIT =
                    CPLD_BASE_ADDR + 0x11,
                /** @brief Register: SD I/O port 0 (shared address with stdout bit).
                 */
                REG_SD_IO_0 = CPLD_BASE_ADDR +
                              0x11,
                /** @brief Register: SD I/O port 1.
                 */
                REG_SD_IO_1 = CPLD_BASE_ADDR + 0x13,
                /** @brief Register: SD I/O port 2.
                 */
                REG_SD_IO_2 = CPLD_BASE_ADDR + 0x15,
                /** @brief Register: SD I/O port 3.
                 */
                REG_SD_IO_3 = CPLD_BASE_ADDR + 0x17,
                /** @brief Register: SD reinsert/eject command.
                 */
                REG_SD_REINSERT =
                    CPLD_BASE_ADDR + 0x19,
                /** @brief Register: SD write-protect / SD present status.
                 */
                REG_SD_WRITE_PROTECT =
                    CPLD_BASE_ADDR +
                    0x1B
            };

            /**
             * @brief Reads an 8-bit CS1 register value from the DevCart CPLD space.
             */
            static inline uint8_t ReadRegister(const Register reg)
            {
                return *(volatile uint8_t *)(static_cast<uint32_t>(reg));
            }

            /**
             * @brief Returns true when the expected CPLD identification bytes are present.
             */
            static inline bool HasWascaSignature()
            {
                return ReadRegister(Register::CPLD_55) == 0x55 &&
                       ReadRegister(Register::CPLD_AA) == 0xAA;
            }

            /**
             * @brief Returns true when cartridge reports USB Gamer's CPLD version.
             */
            static inline bool IsUsbGamersCartridge()
            {
                return ReadRegister(Register::CART_CPLD_VER) == 0x19;
            }

            /**
             * @note `REG_STDOUT_BIT` and `REG_SD_IO_0` share the same address.
             * This suggests they might be bit aliases or their function is mode-dependent.
             * Care should be taken to avoid conflicts when using them.
             */

        } // namespace CS1

        /**
         * @brief Minimal framed protocol for host commands over DevCart USB FIFO.
         *
         * This protocol is used by host tools (such as ftx) to send filesystem-like
         * requests (ls/rm/crc) through FTDI, where Saturn-side code can parse and
         * handle them.
         *
         * Request frame:
         *   - 4 bytes magic: "SRL1"
         *   - 1 byte command
         *   - 2 bytes payload length (big-endian)
         *   - N bytes payload
         *
         * Response frame:
         *   - 4 bytes magic: "SRL1"
         *   - 1 byte status
         *   - 2 bytes payload length (big-endian)
         *   - N bytes payload
         */
        namespace HostIo
        {
            /** @brief Command type identifiers for host-to-Saturn requests.
             */
            enum class Command : uint8_t
            {
                /** @brief List files command. */
                List = 1,
                /** @brief Remove file command. */
                Remove = 2,
                /** @brief Compute CRC checksum command. */
                Crc = 3,
                /** @brief Upload file command. */
                Upload = 4,
                /** @brief Create directory command. */
                Mkdir = 5,
                /** @brief Remove directory command. */
                Rmdir = 6
            };

            /** @brief Status response code identifiers for Saturn-to-host responses.
             */
            enum class Status : uint8_t
            {
                /** @brief Operation succeeded. */
                Ok = 0,
                /** @brief General error occurred. */
                Error = 1,
                /** @brief Command/operation is unsupported. */
                Unsupported = 2,
                /** @brief Bad request parameters or format. */
                BadRequest = 3,
                /** @brief Request successfully handled. */
                Handled = 4
            };

            /** @brief Magic character 0 ('S').
             */
            constexpr static uint8_t MAGIC_0 = 'S';
            /** @brief Magic character 1 ('R').
             */
            constexpr static uint8_t MAGIC_1 = 'R';
            /** @brief Magic character 2 ('L').
             */
            constexpr static uint8_t MAGIC_2 = 'L';
            /** @brief Magic character 3 ('1').
             */
            constexpr static uint8_t MAGIC_3 = '1';

            /** @brief Total size in bytes of the protocol frame header.
             */
            constexpr static size_t HEADER_SIZE = 7;

            /** @brief Write all bytes of a buffer to the DevCart USB FIFO.
             *  @param data Pointer to the source buffer.
             *  @param size Number of bytes to write.
             *  @return True if all bytes were successfully written.
             */
            static inline bool WriteAll(const uint8_t *data, size_t size)
            {
                return CS0::write(data, size) == size;
            }

            /** @brief Read all bytes into a buffer from the DevCart USB FIFO.
             *  @param data Pointer to the destination buffer.
             *  @param size Number of bytes to read.
             *  @return True when all bytes are successfully read.
             */
            static inline bool ReadAll(uint8_t *data, size_t size)
            {
                for (size_t i = 0; i < size; ++i)
                {
                    data[i] = CS0::read();
                }
                return true;
            }

            /** @brief Decodes a 16-bit big-endian value from two bytes.
             *  @param hi High byte.
             *  @param lo Low byte.
             *  @return The decoded 16-bit unsigned integer value.
             */
            static inline uint16_t DecodeU16BE(const uint8_t hi, const uint8_t lo)
            {
                return static_cast<uint16_t>((static_cast<uint16_t>(hi) << 8) |
                                             static_cast<uint16_t>(lo));
            }

            /** @brief Attempt to read and parse a request frame from Host.
             *  @param command Output parameter returning the parsed command.
             *  @param payloadBuffer Buffer to receive the request payload.
             *  @param payloadCapacity Maximum bytes the payloadBuffer can hold.
             *  @param payloadSize Output parameter returning the actual read payload size.
             *  @return True if request header was read successfully and matches protocol rules.
             */
            static inline bool TryReadRequest(Command &command,
                uint8_t *payloadBuffer,
                size_t payloadCapacity,
                size_t &payloadSize)
            {
                payloadSize = 0;
                uint8_t header[HEADER_SIZE];
                if (!ReadAll(header, HEADER_SIZE))
                {
                    return false;
                }

                if (header[0] != MAGIC_0 || header[1] != MAGIC_1 ||
                    header[2] != MAGIC_2 || header[3] != MAGIC_3)
                {
                    return false;
                }

                command = static_cast<Command>(header[4]);
                const uint16_t payloadLen = DecodeU16BE(header[5], header[6]);

                if (payloadLen > payloadCapacity)
                {
                    uint8_t sink = 0;
                    for (uint16_t i = 0; i < payloadLen; ++i)
                    {
                        sink = CS0::read();
                    }
                    (void)sink;
                    return false;
                }

                if (payloadLen > 0)
                {
                    ReadAll(payloadBuffer, payloadLen);
                    payloadSize = payloadLen;
                }

                return true;
            }

            /** @brief Send a protocol response frame to the Host.
             *  @param status The response status code.
             *  @param payload Pointer to response payload buffer (can be nullptr if payloadSize is 0).
             *  @param payloadSize Number of bytes in response payload.
             *  @return True if response was sent successfully.
             */
            static inline bool SendResponse(Status status,
                const uint8_t *payload,
                size_t payloadSize)
            {
                if (payloadSize > 0xFFFFU)
                {
                    return false;
                }

                uint8_t header[HEADER_SIZE] = {
                    MAGIC_0,
                    MAGIC_1,
                    MAGIC_2,
                    MAGIC_3,
                    static_cast<uint8_t>(status),
                    static_cast<uint8_t>((payloadSize >> 8) & 0xFFU),
                    static_cast<uint8_t>(payloadSize & 0xFFU)};

                if (!WriteAll(header, HEADER_SIZE))
                {
                    return false;
                }

                if (payloadSize == 0)
                {
                    return true;
                }

                return WriteAll(payload, payloadSize);
            }
        } // namespace HostIo

    } // namespace DevCart
} // namespace SRL
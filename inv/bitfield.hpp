#include <cstddef>
#include <cassert>
#include <cstdint>

#ifndef BITFIELD_HPP__
#define BITFIELD_HPP__

namespace translib
{

    using namespace std;

    /**
     * @brief Object to act as a replacement for raw bitfields.
     *
     * This class is to build bitfield like objects that could be used by the basecom library.
     *
     * @tparam bitlength
     */
    template <const size_t bitlength>
    class Bitfield
    {
        /**
         * TODO: Allow bits in a field to wrap around the boundaries of the backing type.
         * Currently the boundaries of a bitfield must be within a single backing datatype.
         *
         * Eg if a Bitfield uses 3 Bits starting at the 7th bit the backing datatype must be at least 16bit wide, because on an 8bit data type only the first bit of the field would be within the boundaries.
         *
         */
        /**
         * @brief The backing type for the internal data array.
         *
         * TODO: Try to deduce the most suitable data type at compile time.
         *
         */
        using backingtype = uint8_t;

    public:
        /**
         * @brief The length of the bitfield in bytes
         *
         */
        static const size_t BYTE_LENGTH = (bitlength + 7) / 8;

        /**
         * @brief Get serialized byte length of the object.
         *
         * @return constexpr size_t
         */
        constexpr size_t GetByteLength() const
        {
            return BYTE_LENGTH;
        }

        /**
         * @brief Calculate the bitmask for the affected bits and the shift from the data of the nearest byte start.
         *
         * @param bitstart - The bit in the bitfield where the bitmask should start.
         * @param length - Number of bits to use.
         * @param shift - Shift of the mask from the byte start.
         * @return constexpr backingtype
         */
        static constexpr backingtype CalculateBitMask(const size_t bitstart, const size_t length, size_t &shift)
        {
            shift = bitstart % (sizeof(backingtype) * 8);
            assert(((shift + length) + 7) / 8 <= sizeof(backingtype));
            backingtype mask = 0;
            for (size_t i = shift; i < (length + shift); i++)
            {
                mask |= 1LL << i;
            }
            return mask;
        }

        /**
         * @brief Get the data stored in a bitfield. Casts the output to a prefered data type
         *
         * @tparam T
         * @param bitstart - The offset of the starting bit
         * @param datalength - Bitlength
         * @return T
         */
        template <typename T>
        T GetData(const size_t bitstart, const size_t datalength) const
        {
            static_assert(sizeof(T) <= sizeof(backingtype), "The type of the backing array must be greater or equal the return type size");
            backingtype rawdata = storage[GetStorageLocationForBit(bitstart)];
            backingtype mask;
            size_t shift = 0;
            mask = CalculateBitMask(bitstart, datalength, shift);
            if constexpr (std::is_same<T, bool>::value)
            {
                return ((rawdata & mask) >> shift) != 0;
            }
            else
            {
                return static_cast<T>((rawdata & mask) >> shift);
            }
        }

        /**
         * @brief Write data to the bitfield.
         *
         * @tparam T - This data type must not be greater than the backing data type.
         * @param bitstart
         * @param datalength
         * @param data
         */
        template <typename T>
        void WriteData(const size_t bitstart, const size_t datalength, const T data)
        {
            static_assert(sizeof(T) <= sizeof(backingtype), "The type of the backing array must be greater or equal the return type size");
            backingtype mask;
            size_t shift = 0;
            mask = CalculateBitMask(bitstart, datalength, shift);
            T maskedData;
            if constexpr (std::is_same<T, bool>::value)
            {
                maskedData = (data ? 1 : 0) & (mask >> shift);
            }
            else
            {
                maskedData = data & (mask >> shift);
            }
            storage[GetStorageLocationForBit(bitstart)] &= ~mask;               // Clear all bits within this area
            storage[GetStorageLocationForBit(bitstart)] |= maskedData << shift; // Set bits with new data
        }

        /**
         * @brief Serialize this packet to the supplied iterator
         *
         * @tparam iterator
         * @param start
         * @param end
         * @return size_t
         */
        template <typename iterator>
        size_t BuildPacket(iterator &start, const iterator &end) const
        {
            size_t length = min<size_t>(sizeof(storage), distance(start, end));
            memcpy(&(*start), storage, length);
            advance(start, length);
            return length;
        }

        /**
         * @brief Parses the supplied data as the bitfield and return the number of used bytes
         *
         * @param data
         * @param length
         * @return size_t
         */
        size_t ParseData(const uint8_t *data, const size_t length, bool &valid)
        {
            size_t bytestowrite = sizeof(storage);
            if (length < sizeof(storage))
            {
                valid &= false;
                bytestowrite = length;
            }
            memcpy(storage, data, bytestowrite);
            return bytestowrite;
        }

        bool operator==(const Bitfield &bitfield)
        {
            static_assert(CalculateArrayLength(BYTE_LENGTH) == bitfield.CalculateArrayLength(BYTE_LENGTH), "Bitfields must have the same size");
            bool ret = true;
            for (size_t i = 0; i < CalculateArrayLength(BYTE_LENGTH); i++)
            {
                ret &= storage[i] == bitfield.storage[i];
            }
            return ret;
        }

    protected:
        /**
         * @brief Calculate the index of in the backing array for the supplied bitnumber.
         *
         * @param bitnumber
         * @return constexpr size_t
         */
        static constexpr size_t GetStorageLocationForBit(const size_t bitnumber)
        {
            return bitnumber / (sizeof(backingtype) * 8);
        }

    private:
        /**
         * @brief Calculates the necessary size of the backing array.
         *
         * @param bytelength
         * @return constexpr size_t
         */
        static constexpr size_t CalculateArrayLength(const size_t bytelength)
        {
            return (bytelength + sizeof(backingtype) - 1) / sizeof(backingtype);
        }
        backingtype storage[CalculateArrayLength(BYTE_LENGTH)] = {0};
    };
}
#endif
#include <tuple>
#include <string>
#include <type_traits>
#include "helper.hpp"
#include <array>
#include "bitfield.hpp"

#ifdef USE_MEMALLOC
#include <vector>
#endif

#ifndef COMPACKET_HPP__
#define COMPACKET_HPP__

#define USE_ETL

#ifdef USE_ETL
#include "etl/string.h"
#endif

using namespace std;

namespace translib
{
    /**
     * @brief Namespace for helperfunctions.
     *
     */
    namespace utils
    {
        /**
         * @brief Substitute for non standard strlen_s function, which is not supported in some compilers.
         *
         * @param data
         * @param maxlength
         * @return size_t
         */
        static inline size_t strlen_s(const char *data, size_t maxlength)
        {
            size_t i = 0;
            for (; i < maxlength; i++)
            {
                if (data[i] == 0)
                    break;
            }
            return i;
        }

        /***************************************Calculate Byte Length of different types********************************/

        /**
         * @brief Return the bytesize of the serialized packet.
         *
         * This function calculates the serialized length of the class in the current state.
         *
         * @tparam T
         * @param instance
         * @return constexpr size_t
         */
        template <typename T>
        inline constexpr typename std::enable_if<is_arithmetic_v<remove_reference_t<T>>, size_t>::type
        getSerializedLength(const T &instance)
        {
            return sizeof(T);
        }

        /**
         * @brief Return the bytesize of the serialized packet.
         *
         * Template specialisation for std::string.
         * This returns the current length of the string plus one for the NULL terminator of the string.
         *
         * @tparam
         * @param instance
         * @return size_t
         */
        inline size_t getSerializedLength(const string &instance)
        {
            return instance.length() + 1;
        }

#ifdef USE_ETL
        /**
         * @brief Return the bytesize of the serialized packet.
         *
         * Template specialisation for etl::string.
         * This returns the current length of the string plus one for the NULL terminator of the string.
         *
         * @tparam size_t MAX_SIZE_
         * @param instance
         * @return constexpr size_t
         */
        template <const size_t MAX_SIZE_>
        inline constexpr size_t getSerializedLength(const etl::string<MAX_SIZE_> &instance)
        {
            return instance.length() + 1;
        }
#endif

        /**
         * @brief Return the bytesize of the serialized packet.
         *
         * Template specialisation for bitfields.
         *
         * @tparam size_t MAX_SIZE_
         * @param instance
         * @return constexpr size_t
         */
        template <int bitlength>
        inline constexpr size_t getSerializedLength(const Bitfield<bitlength> &instance)
        {
            return instance.GetByteLength();
        }

        /**
         * @brief Helper struct to calculate the size of an object at compile time.
         *
         * @tparam T
         */
        template <typename T>
        struct max_size
        {
            static const typename std::enable_if<!is_same<T, string>::value, size_t>::type value = sizeof(T);
        };

        /**
         * @brief Helper struct to calculate the size of an object at compile time.
         *
         * @tparam bitlength
         */
        template <int bitlength>
        struct max_size<Bitfield<bitlength>>
        {
            static const size_t value = Bitfield<bitlength>::BYTE_LENGTH;
        };

#ifdef USE_ETL
        /**
         * @brief Helper struct to calculate the size of an object at compile time.
         *
         * @tparam MAX_SIZE_
         */
        template <const size_t MAX_SIZE_>
        struct max_size<etl::string<MAX_SIZE_>>
        {
            static const size_t value = etl::string<MAX_SIZE_>::MAX_SIZE + 1;
        };
#endif

        /**
         * @brief Helper struct to calculate the size of an object at compile time.
         *
         * @tparam Types
         */
        template <typename... Types>
        struct max_byte_length
        {
            static const size_t value = (max_size<Types>::value + ...);
        };
        /***************************************Serialize Message to buffer********************************/

        /**
         * @brief Serialize object to buffer.
         *
         * @tparam type
         * @tparam iterator
         * @param data - Pointer to the data object to serialize.
         * @param it - start iterator pointing to the next free space in the output buffer.
         * @param end - end iterator marking the end of the output buffer.
         * @param tocopy - Max bytes to serialize from the object to the buffer.
         * @return iterator
         */
        template <typename T, typename iterator>
        static inline iterator serializeToBuffer(const T *data, iterator &it, const iterator &end, size_t tocopy)
        {
            static_assert(is_arithmetic_v<remove_reference_t<T>>);
            tocopy = min<size_t>(tocopy, distance(it, end));
            memcpy(&(*it), data, tocopy);
            advance(it, tocopy);
            return it;
        }

        /**
         * @brief Serialize object to buffer.
         *
         * @tparam type
         * @tparam iterator
         * @param data - Pointer to the data object to serialize.
         * @param it - start iterator pointing to the next free space in the output buffer.
         * @param end - end iterator marking the end of the output buffer.
         * @return iterator
         */
        template <typename T, typename iterator>
        static inline constexpr typename std::enable_if<is_arithmetic_v<remove_reference_t<T>>, iterator>::type
        serializeToBuffer(const T &data, iterator &it, const iterator &end)
        {
            size_t typelength = min<size_t>(getSerializedLength(data), distance(it, end));
            return serializeToBuffer(&data, it, end, typelength);
        }

        /**
         * @brief Serialize object to buffer.
         *
         * @tparam iterator
         * @param data - Pointer to the data object to serialize.
         * @param it - start iterator pointing to the next free space in the output buffer.
         * @param end - end iterator marking the end of the output buffer.
         * @return iterator
         */
        template <typename iterator>
        inline iterator serializeToBuffer(const string &data, iterator &it, const iterator &end)
        {
            size_t typelength = min<size_t>(getSerializedLength(data), distance(it, end));
            it = serializeToBuffer(data.c_str(), it, end, typelength - 1);
            *it = 0;
            it++;
            return it;
        }

#ifdef USE_ETL
        /**
         * @brief Serialize object to buffer.
         *
         * @tparam MAX_SIZE_
         * @tparam iterator
         * @param data - Pointer to the data object to serialize.
         * @param it - start iterator pointing to the next free space in the output buffer.
         * @param end - end iterator marking the end of the output buffer.
         * @return iterator
         */
        template <const size_t MAX_SIZE_, typename iterator>
        inline iterator serializeToBuffer(const etl::string<MAX_SIZE_> &data, iterator &it, const iterator &end)
        {
            size_t typelength = min<size_t>(getSerializedLength(data), distance(it, end));
            it = serializeToBuffer(data.c_str(), it, end, typelength - 1);
            *it = 0;
            it++;
            return it;
        }
#endif

        /**
         * @brief Serialize object to buffer.
         *
         * @tparam bitlength
         * @tparam iterator
         * @param data - Pointer to the data object to serialize.
         * @param it - start iterator pointing to the next free space in the output buffer.
         * @param end - end iterator marking the end of the output buffer.
         * @return iterator
         */
        template <const size_t bitlength, typename iterator>
        static inline iterator serializeToBuffer(const Bitfield<bitlength> &data, iterator &it, const iterator &end)
        {
            data.BuildPacket(it, end);
            return it;
        }

        /***************************************Unserialize message from buffer********************************/

        /**
         * @brief Deserialize data from the buffer data to the object element.
         *
         * This function is to deserialize data to plain data types.
         *
         * @tparam type
         * @param data - Buffer with serialzed data.
         * @param length - Number of bytes in the buffer.
         * @param element - The element to unserialize the data to.
         * @param valid - Reference value is set to true if the data in element is considered valid. This is true if there are at least enough bytes in the buffer to fully fill the element.
         * @return size_t
         */
        template <typename T>
        static inline constexpr typename std::enable_if<is_arithmetic_v<remove_reference_t<T>>, size_t>::type
        deserializeFromBuffer(const uint8_t *data, const size_t length, T &element, bool &valid)
        {
            auto byte_size = getSerializedLength(element);
            if (length >= byte_size)
            {
                memcpy(&element, data, byte_size);
                return byte_size;
            }
            else
            {
                valid &= false;
                return length;
            }
        }

        /**
         * @brief Deserialize data from the buffer data to the object element.
         *
         * This function is to deserialize data to std::string.
         *
         * @param data - Buffer with serialzed data.
         * @param length - Number of bytes in the buffer.
         * @param element - The string to unserialize the data to.
         * @param valid - Reference value is set to true if the data in element is considered valid. This is true if there are at least enough bytes in the buffer to fully fill the element.
         * @return size_t
         */
        static inline size_t deserializeFromBuffer(const uint8_t *data, const size_t length, string &element, bool &valid)
        {
            (void)valid;
            size_t stringlength = strlen_s(reinterpret_cast<const char *>(data), length);
            element = string(reinterpret_cast<const char *>(data), stringlength);
            if(stringlength < length) //If we read the last bytes in the string no null terminator is needed for termination, so just return the number of read charakters.
            {
                stringlength++; //If the data is longer than the found string, a nullterminator was present in the string, so mark that as read.
            }
            return stringlength;
        }

#ifdef USE_ETL
        /**
         * @brief Deserialize data from the buffer data to the object element.
         *
         * This function is to deserialize data to etl::string.
         *
         * @tparam MAX_SIZE_
         * @param data - Buffer with serialzed data.
         * @param length - Number of bytes in the buffer.
         * @param element - The string to unserialize the data to.
         * @param valid - Reference value is set to true if the data in element is considered valid. This is true if there are at least enough bytes in the buffer to fully fill the element.
         * @return size_t
         */
        template <const size_t MAX_SIZE_>
        size_t inline deserializeFromBuffer(const uint8_t *data, const size_t length, etl::string<MAX_SIZE_> &element, bool &valid)
        {
            (void)valid;
            size_t stringlength = min<size_t>(strlen_s(reinterpret_cast<const char *>(data), length), MAX_SIZE_);
            element = etl::string<MAX_SIZE_>(reinterpret_cast<const char *>(data), stringlength);
            if(stringlength < length) //If we read the last bytes in the string no null terminator is needed for termination, so just return the number of read charakters.
            {
                stringlength++; //If the data is longer than the found string, a nullterminator was present in the string, so mark that as read.
            }
            return stringlength;
        }
#endif

        /**
         * @brief Deserialize data from the buffer data to the object element.
         *
         * This function deserializes the data to the bitfield data struct.
         *
         * @tparam bitlength
         * @param data - Buffer with serialzed data.
         * @param length - Number of bytes in the buffer.
         * @param element - The string to unserialize the data to.
         * @param valid - Reference value is set to true if the data in element is considered valid. This is true if there are at least enough bytes in the buffer to fully fill the element.
         * @return size_t
         */
        template <const size_t bitlength>
        static inline size_t deserializeFromBuffer(const uint8_t *data, const size_t length, Bitfield<bitlength> &element, bool &valid)
        {
            size_t ret = element.ParseData(data, length, valid);
            return ret;
        }
    }

    /***************************************Base class for all communication packets********************************/

    /**
     * @brief Data structure to be used as a base class for all data packets
     *
     * This class contains all methods and fields to serialize and deserialize data to and from the packet
     *
     * @tparam T
     */
    template <class... T>
    class ComPacket
    {
        using tupletype = tuple<T...>;

    public:
        ComPacket()
        {
        }

        /**
         * @brief Construct a new Com Packet object and initialize them with the provided values.
         *
         * @param values
         */
        ComPacket(T... values)
        {
            elements = tupletype(values...);
        }

        /**
         * @brief Compile time variable if this packet supports the GetMaxSize constexpr.
         *
         * If this field contains the value false a call to GetMaxSize will throw a compile time error.
         * This field could be used in a compile time if constexpr to decide if the packet supports compile time size expressions.
         *
         */
        static const bool SupportsMaxSize = !tuple_helper::tuple_contains_type<string, tupletype>::value;

        /**
         * @brief Get the Max Size of the object
         *
         * @return constexpr size_t
         */
        static constexpr inline size_t GetMaxSize()
        {
            if constexpr (SupportsMaxSize)
            {
                return utils::max_byte_length<T...>::value;
            }
        }

        /**
         * @brief Get the serialzed length of the packet.
         *
         * @return size_t
         */
        size_t GetSerializedLength() const
        {
            return apply([](auto &&...args)
                         { return ((utils::getSerializedLength(args)) + ...); },
                         elements);
        }

        /**
         * @brief Serialize the packet to the buffer starting at begin and ending on end
         *
         * @tparam length
         * @param buffer - The array to hold the data
         * @return true on sucess false otherwise. This function will return false if the buffer is to small to hold the whole packet.
         */
        template <const size_t length>
        size_t Serialze(array<uint8_t, length> &buffer) const
        {
            return Serialze(buffer.begin(), buffer.end());
        }

#ifdef USE_MEMALLOC
        size_t Serialze(vector<uint8_t> &buffer) const
        {
            return Serialze(buffer.begin(), buffer.end());
        }
#endif

        /**
         * @brief Deserialize data from the buffer.
         *
         * This function deserializes the data and returns them as tuple. The tuple has the same structure as this data packet.
         *
         * @param data - The buffer that holds the data.
         * @param length - The length of the buffer.
         * @return tuple that:
         *              -holds the amount of bytes that where read from the buffer.
         *              -a bool if the unserialized packet is valid. This is true if the amount of data in the buffer is at least the minimal length of the bytes in this packet
         *              -a tuple that holds the deserialized values
         */
        static auto Unserialize(const uint8_t *data, size_t length)
        {
            auto parsed_elements = tupletype();
            size_t offset = 0;
            bool valid = apply([&offset, &data, length](auto &&...args)
                               {
                                                 bool valid = true;
                                                 ((offset += utils::deserializeFromBuffer(&data[offset], length - offset, args, valid)), ...);
                                                 return valid; },
                               parsed_elements);
            return make_tuple(offset, valid, parsed_elements);
        }

        /**
         * @brief Deserialize data from the buffer.
         *
         * @param data - The buffer that holds the data.
         * @param length - The length of the buffer.
         * @param packet - The packet the data should be unserialized to.
         * @return a tuple which holds the number of read bytes as a size_t and a boolean that marks if the deserialized data could be valid.
         */
        static auto Unserialize(const uint8_t *data, size_t length, ComPacket<T...> &packet)
        {
            size_t offset;
            bool valid;
            tupletype parsed_elements;
            tie(offset, valid, parsed_elements) = Unserialize(data, length);
            if (valid)
            {
                packet.SetElements(parsed_elements);
            }
            return make_tuple(offset, valid);
        }

        /**
         * @brief Sends the data using a functor.
         *
         * This function causes the object to serialize itself and passes the data to the functor
         *
         * @tparam sendfunctor - The functor to use to send the serialized data.
         * @param send
         */
        template <typename sendfunctor>
        void SendData(sendfunctor &send) const
        {
            if constexpr (SupportsMaxSize)
            {
                array<uint8_t, GetMaxSize()> data;
                Serialze(data.begin(), data.end());
                send(data.begin(), data.end());
            }
            else
            {
                vector<uint8_t> data;
                Serialze(data.begin(), data.end());
                send(data.begin(), data.end());
            }
        }

    protected:
        /**
         * @brief tuple that holds the data of the object.
         *
         */
        tupletype elements;

    private:
        /**
         * @brief Set the Elements object.
         *
         * @param t
         */
        void SetElements(tuple<const T &...> t)
        {
            elements = t;
        }

        /**
         * @brief Serialize the packet to the buffer starting at begin and ending on end
         *
         * @tparam iterator
         * @param begin - Begin of the buffer where the data should be written.
         * @param end
         * @return true on sucess false otherwise. This function will return false if the buffer is to small to hold the whole packet.
         */
        template <typename iterator>
        size_t Serialze(const iterator& begin, const iterator& end) const
        {
            const size_t packetLength = GetSerializedLength();
            const size_t iteratorLength = distance(begin, end);
            if (packetLength > iteratorLength)
            {
                return 0;
            }

            iterator start = begin;

            apply([&start, &end](auto &&...args)
                  { ((utils::serializeToBuffer(args, start, end)), ...); },
                  elements);

            return distance(begin, start);
        }
    };
}
#endif
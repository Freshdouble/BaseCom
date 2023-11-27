#include <cstdint>
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

//#define USE_ETL

#ifdef USE_ETL
#include "etl/string.h"
#include "etl/vector.h"
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
		if (data[i] == 0) break;
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
template<typename T>
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

template<typename T, size_t length>
inline constexpr size_t getSerializedLength(const std::array<T, length> &data)
{
	size_t sum = 0;
	for (const auto &d : data)
	{
		sum += getSerializedLength(d);
	}
	return sum;
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
template<const size_t MAX_SIZE_>
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
template<const size_t bitlength>
inline constexpr size_t getSerializedLength(const Bitfield<bitlength> &instance)
{
	return instance.GetByteLength();
}

/**
 * @brief Helper struct to calculate the size of an object at compile time.
 *
 * @tparam T
 */
template<typename T>
struct max_size
{
	static const typename std::enable_if<!is_same<T, string>::value, size_t>::type value = sizeof(T);
};

/**
 * @brief Helper struct to calculate the size of an object at compile time.
 *
 * @tparam bitlength
 */
template<int bitlength>
struct max_size<Bitfield<bitlength>>
{
	static const size_t value = Bitfield<bitlength>::BYTE_LENGTH;
};

template<typename T, size_t length>
struct max_size<std::array<T, length>>
{
	static const size_t value = max_size<T>::value * length;
};

#ifdef USE_ETL
/**
 * @brief Helper struct to calculate the size of an object at compile time.
 *
 * @tparam MAX_SIZE_
 */
template<const size_t MAX_SIZE_>
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
template<typename ... Types>
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
template<typename T, typename iterator>
static inline iterator serializeToBuffer(const T *data, iterator &it, const iterator &end, size_t tocopy)
{
	static_assert(is_arithmetic_v<remove_reference_t<T>>);
	auto dist = distance(it, end);
	assert(dist >= 0);
	tocopy = min<size_t>(tocopy, dist);
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
template<typename T, typename iterator>
static inline constexpr typename std::enable_if<is_arithmetic_v<remove_reference_t<T>>, iterator>::type
serializeToBuffer(const T &data, iterator &it, const iterator &end)
{
	auto dist = distance(it, end);
	assert(dist >= 0);
	size_t typelength = min<size_t>(getSerializedLength(data), dist);
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
template<typename iterator>
inline iterator serializeToBuffer(const string &data, iterator &it, const iterator &end)
{
	auto dist = distance(it, end);
	assert(dist >= 0);
	size_t typelength = min<size_t>(getSerializedLength(data), dist);
	it = serializeToBuffer(data.c_str(), it, end, typelength - 1);
	*it = 0;
	it++;
	return it;
}

template<typename arraytype, size_t length, typename iterator>
inline iterator serializeToBuffer(const std::array<arraytype, length> &data, iterator &it, const iterator &end)
{
	for (size_t i = 0; i < length; i++)
	{
		it = serializeToBuffer(data[i], it, end);
	}
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
template<const size_t MAX_SIZE_, typename iterator>
inline iterator serializeToBuffer(const etl::string<MAX_SIZE_> &data, iterator &it, const iterator &end)
{
	auto dist = distance(it, end);
	assert(dist >= 0);
	size_t typelength = min<size_t>(getSerializedLength(data), dist);
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
template<const size_t bitlength, typename iterator>
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
template<typename T>
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
	(void) valid;
	size_t stringlength = strlen_s(reinterpret_cast<const char*>(data), length);
	element = string(reinterpret_cast<const char*>(data), stringlength);
	if (stringlength < length) // If we read the last bytes in the string no null terminator is needed for termination, so just return the number of read charakters.
	{
		stringlength++; // If the data is longer than the found string, a nullterminator was present in the string, so mark that as read.
	}
	return stringlength;
}

template<typename T, size_t arraylength>
static inline size_t deserializeFromBuffer(const uint8_t *data, const size_t length, std::array<T, arraylength> &element, bool &valid)
{
	auto byte_size = getSerializedLength(element);
	size_t readbytes = 0;
	if (length >= byte_size)
	{
		for (size_t i = 0; i < arraylength; i++)
		{
			readbytes += deserializeFromBuffer(&data[readbytes], length - readbytes, element[i], valid);
		}
		return readbytes;
	}
	else
	{
		valid &= false;
	}
	return length;
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
template<const size_t MAX_SIZE_>
size_t inline deserializeFromBuffer(const uint8_t *data, const size_t length, etl::string<MAX_SIZE_> &element, bool &valid)
{
	(void) valid;
	size_t stringlength = min<size_t>(strlen_s(reinterpret_cast<const char*>(data), length), MAX_SIZE_);
	element = etl::string<MAX_SIZE_>(reinterpret_cast<const char*>(data), stringlength);
	if (stringlength < length) // If we read the last bytes in the string no null terminator is needed for termination, so just return the number of read charakters.
	{
		stringlength++; // If the data is longer than the found string, a nullterminator was present in the string, so mark that as read.
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
template<const size_t bitlength>
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
template<class ... T>
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
	ComPacket(T ... values)
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
	static constexpr size_t GetMaxSize()
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
		{	return ((utils::getSerializedLength(args)) + ...);}, elements);
	}

	/**
	 * @brief Serialize the packet to the buffer with the specified id data before the serialized data.
	 *
	 * @tparam datalength
	 * @tparam idlength
	 * @param buffer
	 * @param idbytes
	 * @return size_t
	 */
	template<const size_t datalength, const size_t idlength>
	size_t Serialize(array<uint8_t, datalength> &buffer, const array<uint8_t, idlength> &idbytes) const
	{
		if constexpr (SupportsMaxSize)
		{
			static_assert(datalength >= (idlength + GetMaxSize()), "The output buffer must be large enough to contain the whole packet");
		}
		else
		{
			static_assert(datalength >= idlength, "The output buffer must be large enough to contain atleast the id");
		}
		auto it = copy(idbytes.begin(), idbytes.end(), buffer.begin());
		return Serialize(it, buffer.end()) + idlength;
	}

	/**
	 * @brief Serialize the packet to the buffer with the specified id data before the serialized data.
	 *
	 * @tparam idlength
	 * @param buffer
	 * @param idbytes
	 * @return size_t
	 */
	template<const size_t idlength>
	size_t Serialize(etl::ivector<uint8_t> &buffer, const array<uint8_t, idlength> &idbytes) const
	{
		if constexpr (SupportsMaxSize)
		{
			assert(buffer.max_size() >= idlength + GetMaxSize());
			if (!(buffer.max_size() >= idlength + GetMaxSize())) return 0;
			buffer.resize(idlength + GetMaxSize());
		}
		else
		{
			assert(buffer.max_size() >= idlength + GetSerializedLength());
			if (!(buffer.max_size() >= idlength + GetSerializedLength())) return 0;
			buffer.resize(idlength + GetSerializedLength());
		}
		auto it = copy(idbytes.begin(), idbytes.end(), buffer.begin());
		return Serialize(it, buffer.end()) + idlength;
	}

#ifdef USE_MEMALLOC
        /**
         * @brief Serialize the packet to the buffer with the specified id data before the serialized data.
         *
         * @tparam idlength
         * @param buffer
         * @param idbytes
         * @return size_t
         */
        template <const size_t idlength>
        size_t Serialize(vector<uint8_t> &buffer, const array<uint8_t, idlength> &idbytes) const
        {
            if constexpr (SupportsMaxSize)
            {
            	buffer.reserve(idlength + GetMaxSize());
                buffer.resize(idlength + GetMaxSize());
            }
            else
            {
            	buffer.reserve(idlength + GetSerializedLength());
                buffer.resize(idlength + GetSerializedLength());
            }
            auto it = copy(idbytes.begin(), idbytes.end(), buffer.begin());
            return Serialize(it, buffer.end());
        }

        /**
         * @brief Serialize the packet to an newly created vector with the specified id data before the serialized data.
         *
         * @tparam idlength
         * @param idbytes
         * @return unique_ptr<vector<uint8_t>>
         */
        template <const size_t idlength>
        unique_ptr<vector<uint8_t>> Serialize(const array<uint8_t, idlength> &idbytes) const
        {
            unique_ptr<vector<uin8_t>> ret = new vector<uint8_t>(idlength + GetSerializedLength());
            auto it = copy(idbytes.begin(), idbytes.end(), ret.begin());
            Serialize(it, ret.end());
            return ret;
        }
#endif

	/**
	 * @brief Check if the id data at the packet start matches with the provided id.
	 *
	 * @tparam datalength
	 * @tparam arraylength
	 *  @param data
	 *  @param length
	 *  @param idbytes
	 *  @return tuple<bool, iterator, size_t> If the packet start matches the id; The beginn of the data section; The remaining bytes in the packet.
	 */
	template<const size_t datalength, const size_t arraylength>
	static tuple<bool, typename array<uint8_t, datalength>::const_iterator, size_t> CheckIDMatch(const std::array<uint8_t, datalength> &data, size_t length, const array<uint8_t, arraylength> &idbytes)
	{
		assert(length <= datalength);
		if (length <= datalength)
		{
			auto [valid, dataptr, remainingbytes] = CheckIDMatch(data.data(), length, idbytes);
			auto readbytes = length - remainingbytes;
			assert(readbytes <= datalength);
			if (valid)
			{
				return make_tuple(true, data.begin() + readbytes, remainingbytes);
			}
		}
		return make_tuple(false, data.begin(), 0);
	}

	/**
	 * @brief Deserialize data from the buffer.
	 *
	 * @tparam maxdatalength
	 * @param it - Start of the data.
	 * @param end - End of the array.
	 * @param length - The length of the buffer.
	 * @param packet - The packet the data should be unserialized to.
	 * @return a tuple which holds the number of read bytes as a size_t, a boolean that marks if the deserialized data could be valid and an iterator past the packet.
	 */
	template<const size_t maxdatalength>
	static auto Unserialize(typename std::array<uint8_t, maxdatalength>::const_iterator it, const typename std::array<uint8_t, maxdatalength>::const_iterator end, size_t length,
			ComPacket<T...> &packet)
	{
		assert(length <= maxdatalength);
		auto dist = std::distance(it, end);
		assert(dist >= 0);
		assert(length <= static_cast<size_t>(dist));
		if (length <= maxdatalength && length <= static_cast<size_t>(dist))
		{

			auto parsed_elements = tupletype();
			size_t offset = 0;
			const uint8_t *data = &(*it);
			bool valid = apply([&offset, &data, length](auto &&...args)
			{
				bool valid = true;
				((offset += utils::deserializeFromBuffer(&data[offset], length - offset, args, valid)), ...);
				return valid;}, parsed_elements);
			if (valid)
			{
				packet.elements = parsed_elements;
			}
			return make_tuple(offset, valid, it + offset);
		}
		else
		{
			return make_tuple(0U, false, it);
		}
	}

	/**
	 * @brief Deserialize data from the buffer.
	 *
	 * @tparam maxdatalength
	 * @param data
	 * @param length
	 * @param packet
	 * @return a tuple which holds the number of read bytes as a size_t, a boolean that marks if the deserialized data could be valid and an iterator past the packet.
	 */
	template<const size_t maxdatalength>
	static auto Unserialize(std::array<uint8_t, maxdatalength> &data, size_t length, ComPacket<T...> &packet)
	{
		return Unserialize<maxdatalength>(data.begin(), data.end(), length, packet);
	}

	/**
	 * @brief Deserialize data from the buffer to this instance
	 *
	 * @tparam maxdatalength
	 * @param it - Start of the data.
	 * @param end - End of the array.
	 * @param length - The length of the buffer.
	 * @return a tuple which holds the number of read bytes as a size_t and a boolean that marks if the deserialized data could be valid.
	 */
	template<const size_t maxdatalength>
	auto Unserialize(typename std::array<uint8_t, maxdatalength>::const_iterator it, const typename std::array<uint8_t, maxdatalength>::const_iterator end, size_t length)
	{
		return Unserialize<maxdatalength>(it, end, length, *this);
	}

	/**
	 * @brief Deserialize data from the buffer to this instance
	 *
	 * @tparam maxdatalength
	 * @param data - The array holding the data
	 * @param length - The length of the buffer.
	 * @return a tuple which holds the number of read bytes as a size_t and a boolean that marks if the deserialized data could be valid.
	 */
	template<const size_t maxdatalength>
	auto Unserialize(const std::array<uint8_t, maxdatalength> &data, size_t length)
	{
		return Unserialize<maxdatalength>(data.begin(), data.end(), length, *this);
	}

protected:
	/**
	 * @brief tuple that holds the data of the object.
	 *
	 */
	tupletype elements;

	/**
	 * @brief Check if the id data at the packet start matches with the provided id.
	 *
	 * @tparam arraylength
	 * @param data
	 * @param datalength
	 * @param idbytes
	 * @return tuple<bool, uint8_t*, size_t> If the packet start matches the id; The beginn of the data section; The remaining bytes in the packet.
	 */
	template<const size_t arraylength>
	static tuple<bool, const uint8_t*, size_t> CheckIDMatch(const uint8_t *data, size_t datalength, const array<uint8_t, arraylength> &idbytes)
	{
		if (datalength < arraylength)
		{
			return make_tuple(false, nullptr, 0);
		}
		bool ret = true;
		// Check if data starts with idbytes
		size_t i;
		for (i = 0; i < arraylength; i++)
		{
			if (idbytes[i] != data[i])
			{
				ret = false;
				break;
			}
		}
		return make_tuple(ret, &data[i], static_cast<size_t>(datalength - i));
	}

private:
	/**
	 * @brief Set the Elements object.
	 *
	 * @param t
	 */
	void SetElements(tuple<const T&...> t)
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
	template<typename iterator>
	size_t Serialize(const iterator &begin, const iterator &end) const
	{
		const size_t packetLength = GetSerializedLength();
		const auto iteratorLength = distance(begin, end);
		assert(iteratorLength >= 0);
		if (iteratorLength < 0 || packetLength > static_cast<size_t>(iteratorLength))
		{
			return 0;
		}

		iterator start = begin;

		apply([&start, &end](auto &&...args)
		{	((utils::serializeToBuffer(args, start, end)), ...);}, elements);

		auto ret = distance(begin, start);
		assert(ret >= 0);
		return ret;
	}
};

template<const size_t idLength, class ...T>
class TagedComPacket: public ComPacket<T...>
{
public:

	TagedComPacket()
	{
		id.fill(0);
	}

	template<typename idlisttype>
	TagedComPacket(std::initializer_list<idlisttype> &&id)
	{
		SetID(id);
	}

	template<typename idlisttype>
	TagedComPacket(std::initializer_list<idlisttype> &id)
	{
		SetID(id);
	}

	template<typename idlisttype>
	void SetID(idlisttype &id)
	{
		auto it = id.begin();
		for (size_t i = 0; i < std::min<size_t>(id.size(), idLength); i++)
		{
			this->id[i] = *it;
			it++;
		}
	}

	template<typename idlisttype>
	void SetID(idlisttype &&id)
	{
		auto it = id.begin();
		for (size_t i = 0; i < std::min<size_t>(id.size(), idLength); i++)
		{
			this->id[i] = *it;
			it++;
		}
	}

	static constexpr size_t GetMaxSize()
	{
		return ComPacket<T...>::GetMaxSize() + idLength;
	}

	template<const size_t datalength>
	auto CheckIDMatch(const std::array<uint8_t, datalength> &data, size_t length) const
	{
		return CheckIDMatch(data, length, id);
	}

	/**
	 * @brief Serialize the packet to the buffer with the specified id data before the serialized data.
	 *
	 * @tparam datalength
	 * @tparam idlength
	 * @param buffer
	 * @param idbytes
	 * @return size_t
	 */
	template<const size_t datalength>
	auto Serialize(array<uint8_t, datalength> &buffer) const
	{
		return ComPacket<T...>::template Serialize<datalength, idLength>(buffer, id);
	}
#ifdef USE_ETL
	auto Serialize(etl::ivector<uint8_t> &buffer) const
	{
		return ComPacket<T...>::template Serialize<idLength>(buffer, id);
	}
#endif

private:

	std::array<uint8_t, idLength> id;
};
}
#endif

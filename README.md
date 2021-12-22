# BASECOM Library

The bascom library is a header only C++ library to define datapackets as well as serialize and deserialize them.
It is intended for an easy straight forward definition of telemetry packets in C++.

The intended use of this library is to receive and transmit data to and from an embedded system. The library would not use dynamic memory allocation and the function would not throw any exceptions.

The only part of the library that would use dynamic memory allocation is the use of std::string in the packets.  A supplement for std::string is etl::string of the embedded template library that is provided in the etl subrepository.

## Disclaimer

This is a library in progress, so not every feature is tested yet.

## Supported C++ version

This library uses C++17 features so the minimum supported C++ language standard is C++17. Later implementations are not tested at this time but should work.

## Compiling

No compilation is needed before using this library, just add the include directory to the compilers header search path and include the BaseCom.hpp header file.

## Usage

To define datapackets create a struct or class and inherit them from the ComPacket class.

```cpp
struct Message : public ComPacket<uint8_t, uint16_t, int, long>
{
    uint8_t& var1 = get<0>(elements);
    uint16_t& var2 = get<1>(elements);
    int& var3 = get<2>(elements);
    long& var4 = get<3>(elements);
};
```

This defines a data packet with an uint8 uint16, int and long. The elements are stored in the internal tuple elements. This layout allows the data fields to be used like in an normal struct. To read or write var1 use

```cpp
Message msg;
msg.var1 = 10;
uint8_t var1 = msg.var1;
```

### Note

ComPacket only supports plain datatypes like int, float and so on. The only classes that are supported are the Bitfield class described later, string and the etl::string class. The etl::string class is a replacement for the std::string class that doesn't use runtime allocation of memory and is supposed to be used in freestanding applications and baremetal programming.

### Bitfields

The basecom library supports a bitfield class to be used in the ComPacket structure. Normal bitfields could be used but only as long as they can be copied from and to by there base address and memcpy. Scince pack instructions are not compiler independend those simple bitfields are not intended for use with this library but they will propably work. If you using simple bitfields to transmit packets over to another machine with another architecture you should pack those bitfields to remove any platform specific padding.

```cpp
struct TestBitfield : public Bitfield<1>
{
    static const size_t TestBit_Offset = 0;
    static const size_t TestBit_Length = 1;

    uint8_t ReadTestBit()
    {
        return GetData<uint8_t>(TestBit_Offset, TestBit_Length);
    }
    void WriteTestBit(uint8_t data)
    {
        return WriteData(TestBit_Offset, TestBit_Length, data);
    }
};
```

This defines a compacket bitfield with a length of 1 bit. By defining the ReadTestBit and WriteTestBit helper functions the bits could be manipulated by an easy to use setter/getter function.

Due to current implementation bitfields must allign the bits that a field inside the bifield wouldn't spread accross the boundarys of the backing type. The backing type usualy is uint8 so a field with the bitoffset of 7 could only be 1 bit long to work properly.

This is a work in progress and the final implementation should support fields that span accross the boundaries of the backing type or even be larger than the backing type.

To use a Bitfield in a compacket it included and used like the other datatypes.

```cpp
struct Message : public ComPacket<uint8_t, uint16_t, int, long, TestBitfield>
{
    uint8_t& var1 = get<0>(elements);
    uint16_t& var2 = get<1>(elements);
    int& var3 = get<2>(elements);
    long& var4 = get<3>(elements);
    TestBitfield& bitfield = get<4>(elements);
};
```

## Serialize and deserialze packets

To serialize a packet the Serialze function must be called with an appropriate buffer, this could be an std::vector if dynamic memory allocation is allowed an compiled in the library or std::array.

```cpp
Message msg;
if constexpr (msg.SupportsMaxSize)
{
    std::array<uint8_t, msg.GetMaxSize()> dataarray;
    size_t serializedlength = msg.Serialze(dataarray);
}
else
{
    //This only works if dynamic memory allocation is implemented
    std::vector<uint8_t> dataarray;
    size_t serializedlength = msg.Serialze(dataarray);
}
```

if you use a structure with an max size that could not be calculated at compile time but would not use std::vector you can use std::array but with an size that would be large enough to hold the data.

Deserializing works in a similar way:

```cpp
Message msg;
size_t usedData;
bool valid;
std::tie(usedData, valid) = Message::Unserialize(dataarray.data(), dataarray.size(), msg);
```

This deserializes the data stored in the data array to the Message msg. usedData is the amount of bytes that where read from the buffer and valid is an indication if the received data could be valid. If the serialized length of the message is longer than the amount of data in the dataarray this is set to false, to indicate that not all fields will have valid data stored in them. Also it is likely that the data stored in the array are not valid data for this message structure.

# Other projects used

This library uses the Embedded Template Library by John Wellbelove. This library contains different implementations for some well known template containers of the standard library that are designed for deterministic behaviour and limited ressources. They wouldn't use any runtime memory allocation so they could be used in baremetal applications.

[Embedded template library website](https://www.etlcpp.com "ETL Hompage")

[Embedded template library github page](https://github.com/ETLCPP/etl "ETL github")

The primary reason for writing this library is to use it as part of the telemetry software stack at the [TU Wien Space Team](https://spaceteam.at/?lang=en).

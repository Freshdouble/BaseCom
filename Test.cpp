#include <iostream>
#include "BaseCom.hpp"
#include <array>

using namespace std;
using namespace translib;

struct PlainTestField : public ComPacket<uint8_t, uint16_t, int, long>
{
    uint8_t &var1 = get<0>(elements);
    uint16_t &var2 = get<1>(elements);
    int &var3 = get<2>(elements);
    long &var4 = get<3>(elements);
};

struct TestBitfield : public Bitfield<1> // Should always allocate one instance of the backing type
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

struct LargeBitField : public Bitfield<70> // Should always allocate at least two instances of the backing data type
{
    static const size_t TestBit1_Offset = 0;
    static const size_t TestBit1_Length = 5;
    static const size_t TestBit2_Offset = 13;
    static const size_t TestBit2_Length = 3;

    uint8_t ReadTestBit1()
    {
        return GetData<uint8_t>(TestBit1_Offset, TestBit1_Length);
    }
    void WriteTestBit1(uint8_t data)
    {
        return WriteData(TestBit1_Offset, TestBit1_Length, data);
    }

    uint8_t ReadTestBit2()
    {
        return GetData<uint8_t>(TestBit2_Offset, TestBit2_Length);
    }
    void WriteTestBit2(uint8_t data)
    {
        return WriteData(TestBit2_Offset, TestBit2_Length, data);
    }
};

struct MixedDataMessage : public ComPacket<int32_t, etl::string<10>, TestBitfield, LargeBitField, std::array<uint8_t, 10>>
{
    int &Testfield1 = get<0>(elements);
    etl::string<10> &Testfield2 = get<1>(elements);
    TestBitfield &Testfield3 = get<2>(elements);
    LargeBitField &Testfield4 = get<3>(elements);
    std::array<uint8_t, 10> &TestArray = get<4>(elements);
};

int main(void)
{
    PlainTestField plaintest;
    plaintest.var1 = 10;
    plaintest.var2 = 100;
    assert(plaintest.var3 == 0 && plaintest.var1 == 10 && plaintest.var2 == 100);

    TestBitfield bitfieldtest;
    bitfieldtest.WriteTestBit(1);
    assert(bitfieldtest.ReadTestBit() == 1);

    bitfieldtest.WriteTestBit(5);
    assert(bitfieldtest.ReadTestBit() == 1);

    bitfieldtest.WriteTestBit(4);
    assert(bitfieldtest.ReadTestBit() == 0);

    LargeBitField largebitfield;

    MixedDataMessage mixed;
    mixed.Testfield1 = -10;
    const etl::string<10> teststring("HELLO WORLD");
    mixed.Testfield2 = teststring;
    mixed.Testfield3.WriteTestBit(1);
    mixed.TestArray.fill(5);

    std::array<uint8_t, 2> id = {2,3};

    std::array<uint8_t, mixed.GetMaxSize() + sizeof(id)> dataarray;
    size_t packageLength = mixed.Serialize(dataarray, id);

    assert(packageLength == (sizeof(int) + (mixed.Testfield2.size() + 1) + TestBitfield::BYTE_LENGTH + LargeBitField::BYTE_LENGTH + sizeof(id) + mixed.TestArray.size() * sizeof(uint8_t)));

    MixedDataMessage deserializeTest;
    size_t usedData;
    auto [valid, packetStart, datalength] = MixedDataMessage::CheckIDMatch(dataarray, packageLength, id);
    assert(valid);

    typename std::array<uint8_t, mixed.GetMaxSize() + sizeof(id)>::const_iterator it;
    std::tie(usedData, valid, it) = deserializeTest.Unserialize<dataarray.max_size()>(packetStart, dataarray.end(), datalength);
    assert(valid);
    assert(deserializeTest.Testfield1 == -10);
    assert(deserializeTest.Testfield2 == teststring);
    assert(deserializeTest.Testfield3.ReadTestBit() == 1);
    for(const auto& d : deserializeTest.TestArray)
    {
        assert(d == 5);
    }

    std::array<uint8_t, 6> falseData = {10,10,20,20,30,30};
    decltype(falseData.cbegin()) falseIterator;
    std::tie(usedData, valid, falseIterator) = MixedDataMessage::Unserialize<falseData.max_size()>(falseData, falseData.max_size(), deserializeTest);
    assert(!valid);

    return 0;
}
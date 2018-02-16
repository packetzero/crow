#include <gtest/gtest.h>
#include "../include/simplepb.hpp"

void HexStringToVec(const std::string str, Bytes &dest);

class DecTest : public ::testing::Test {
 protected:
  virtual void SetUp() {

  }
};

TEST_F(DecTest, smallUnsignedInt) {
  auto vec = std::vector<uint8_t>(3);
  vec[0] = 3;
  vec[1] = TYPE_UINT32;
  vec[2] = 127;

  auto dl = simplepb::DecoderLogger();
  auto &dec = *simplepb::DecoderNew();
  dec.decode(vec, dl);

  ASSERT_EQ("[ 3] uint32 127,", dl.str());

  delete &dec;
}

TEST_F(DecTest, u32) {
  auto vec = std::vector<uint8_t>();
  // fieldIndex=1, type:uint32_t, value=0x8000FFFF (2147549183)
  HexStringToVec("0102ffff838008", vec);

  auto dl = simplepb::DecoderLogger();
  auto &dec = *simplepb::DecoderNew();
  dec.decode(vec, dl);

  ASSERT_EQ("[ 1] uint32 2147549183,", dl.str());

}

TEST_F(DecTest, dub) {
  auto vec = std::vector<uint8_t>();
  HexStringToVec("090577be9f1a2fdd5e40", vec);

  auto dl = simplepb::DecoderLogger();
  auto &dec = *simplepb::DecoderNew();
  dec.decode(vec, dl);

  ASSERT_EQ("[ 9] double 123.456,", dl.str());

}

TEST_F(DecTest, simple) {
  // A a = { 47, 65535, "hello", 123.456};
  auto vec = std::vector<uint8_t>();
  HexStringToVec("00015e0104ffff0302070568656c6c6f030577be9f1a2fdd5e40", vec);

  auto dl = simplepb::DecoderLogger();
  auto &dec = *simplepb::DecoderNew();
  dec.decode(vec, dl);

  ASSERT_EQ("[ 0] int32 47,[ 1] uint64 65535,[ 2] \"hello\",[ 3] double 123.456,", dl.str());

  //delete &dec;
}



uint8_t hexDigitValue(char c)
{
  if (c >= 'a') {
    return (uint8_t)(c - 'a') + 10;
  } else if (c >= 'A') {
    return (uint8_t)(c - 'A') + 10;
  }
  return (uint8_t)(c - '0');
}

void HexStringToVec(const std::string str, Bytes &dest)
{
  for (int i=0;i < str.length(); i+=2) {
    uint8_t val = hexDigitValue(str[i]) << 4 | hexDigitValue(str[i+1]);
    dest.push_back(val);
  }
}

#include <gtest/gtest.h>
#include "../include/crow.hpp"

void BytesToHexString(const unsigned char *bytes, size_t len, std::string &dest);
void BytesToHexString(const Bytes &vec, std::string &dest);
void BytesToHexString(const std::string &bytes, std::string &dest);

class EncTest : public ::testing::Test {
 protected:
  virtual void SetUp() {

  }
};

TEST_F(EncTest, u32) {
  auto pEnc = crow::EncoderNew();
  auto &enc = *pEnc;

  uint32_t val = 7;
  enc.put(0, val);
  const uint8_t* result = enc.data();
  ASSERT_EQ(3, enc.size());
  ASSERT_EQ(0, result[0]);
  ASSERT_EQ(2, result[1]);
  ASSERT_EQ(7, result[2]);

  enc.clear();

  val = 0x8000FFFFU;
  enc.put(1, val);
  result=enc.data();
  ASSERT_EQ(7, enc.size());
  ASSERT_EQ(1, result[0]);
  ASSERT_EQ(2, result[1]);

  delete pEnc;
}

TEST_F(EncTest, withColumnNames) {
  auto pEnc = crow::EncoderNew();
  auto &enc = *pEnc;
  
  uint32_t val = 77;
  enc.put(0, val, "first");
  enc.put(1,"no space to rent in this town", "second");
  const uint8_t* result = enc.data();
  
  std::string s;
  BytesToHexString(result, enc.size(), s);
  
  //    80 name.length name
  // 00 82 05 6669727374    4d
  // 01 87 06 7365636f6e64  1d 6e6f20737061636520746f2072656e7420696e207468697320746f776e
  
  ASSERT_EQ("00820566697273744d0187067365636f6e641d6e6f20737061636520746f2072656e7420696e207468697320746f776e", s);
//  printf("%s\n", s.c_str());
  
//  ASSERT_EQ(3, enc.size());
  
  delete pEnc;
}



TEST_F(EncTest, i32) {
  auto pEnc = crow::EncoderNew();
  auto &enc = *pEnc;

  int32_t val = -2;
  enc.put(0, val);
  auto result = enc.data();
  ASSERT_EQ(3, enc.size());
  ASSERT_EQ(0, result[0]);
  ASSERT_EQ(1, result[1]);
  ASSERT_EQ(3, result[2]);  // ZigZagEncode32(-2) == 3u

  std::string s;
  BytesToHexString(result, enc.size(), s);
  //printf("%s\n", s.c_str());

  delete pEnc;
}

TEST_F(EncTest, dub) {
  auto pEnc = crow::EncoderNew();
  auto &enc = *pEnc;

  double val = 123.456;
  enc.put(9, val);
  auto result = enc.data();

  ASSERT_EQ(10, enc.size());
  ASSERT_EQ(9, result[0]);
  ASSERT_EQ(TYPE_DOUBLE, result[1]);

  std::string s;
  BytesToHexString(result, enc.size(), s);
  //printf("%s\n", s.c_str());

  delete pEnc;
}

TEST_F(EncTest, repeatedStrings) {
  auto pEnc = crow::EncoderNew();
  auto &enc = *pEnc;

  uint32_t fieldIndex = 3;
  enc.put(fieldIndex, "one");
  enc.put(fieldIndex, std::string("two"));
  enc.put(fieldIndex, "three");
  enc.put(fieldIndex, "four");
  enc.put(fieldIndex, "five");
  auto result = enc.data();

  std::string exp="\x03\a\x03one\x03\a\x03two\x03\a\x05three\x03\a\04four\x03\a\04five";
  ASSERT_EQ(exp.length(), enc.size());
  ASSERT_TRUE(memcmp(exp.c_str(), result, exp.length())==0);

  std::string s;
  BytesToHexString(result, enc.size(), s);
  //printf("%s\n", s.c_str());

  delete pEnc;
}


TEST_F(EncTest, simple) {
  struct A {
    int32_t     i32val;
    uint64_t    u64val;
    std::string strval;
    double      dval;
  };

  A a = { 0x2F, 0x0FFFF, "hello", 123.456};
  auto pEnc = crow::EncoderNew();
  auto &enc = *pEnc;

  uint32_t fieldIndex = 0;
  enc.put(fieldIndex++, a.i32val);  // 2 + 2
  enc.put(fieldIndex++, a.u64val);  // 2 + 5
  enc.put(fieldIndex++, a.strval);  // 2 + 1 + 5
  enc.put(fieldIndex++, a.dval);   // 2 + 8
  auto result = enc.data();

  std::string s;
  BytesToHexString(result, enc.size(), s);
  //printf("%s\n", s.c_str());

  ASSERT_EQ(26, enc.size());

  delete pEnc;
}

static const char hexCharsLower[] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};
static const char hexCharsUpper[] = {
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};


void BytesToHexString(const unsigned char *bytes, size_t len, std::string &dest)
{
  static bool useLowerCase = true;
  const char *hexChars = hexCharsLower;
  if (!useLowerCase) hexChars = hexCharsUpper;

  const unsigned char *src = bytes;
  size_t offset = dest.length();
  dest.resize(offset + len*2);
  for (size_t i=0;i < len; i++) {
    char b = *src++;
    dest[offset++] = hexChars[ ((b >> 4) & 0x0F) ];
    dest[offset++] = hexChars[ (b & 0x0F) ];
  }
}

void BytesToHexString(const Bytes &bytes, std::string &dest)
{
  BytesToHexString(bytes.data(), bytes.size(), dest);
}

void BytesToHexString(const std::string &bytes, std::string &dest)
{
  BytesToHexString((const unsigned char *)bytes.data(), bytes.size(), dest);
}

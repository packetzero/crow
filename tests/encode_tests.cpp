#include <gtest/gtest.h>
#include "../include/crow.hpp"

void BytesToHexString(const unsigned char *bytes, size_t len, std::string &dest);
void BytesToHexString(const Bytes &vec, std::string &dest);
void BytesToHexString(const std::string &bytes, std::string &dest);

#define ENC_GTEST_LOG_ENABLED 0

enum MY_FIELDS {
  MY_FIELD_A = 2,
  MY_FIELD_B = 54,
  MY_FIELD_C = 102
};

class EncTest : public ::testing::Test {
 protected:
  virtual void SetUp() {

  }
};


TEST_F(EncTest, withColumnNames) {
  auto pEnc = crow::EncoderNew();
  auto &enc = *pEnc;

  std::string s = "";

  PUT_NAME(pEnc, "name", "bob");     s += "01008100046e616d6503626f62";
  PUT_NAME(pEnc, "age", 23);         s += "01018200036167652e";
  PUT_NAME(pEnc, "active", true);    s += "010289000661637469766501";
  enc.putRowSep();                   s += "03";

  PUT_NAME(pEnc, "name","jerry");    s += "80056a65727279";
  PUT_NAME(pEnc, "age", 58) ;        s += "8174";
  PUT_NAME(pEnc, "active", false);   s += "8200";
  enc.putRowSep();                   s += "03";

  PUT_NAME(pEnc, "name", "linda");   s += "80056c696e6461";
  PUT_NAME(pEnc, "age", 33) ;        s += "8142";
  PUT_NAME(pEnc, "active", true);    s += "8201";

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}

TEST_F(EncTest, encodeFloats)
{
  auto pEnc = crow::EncoderNew();
  auto &enc = *pEnc;

  std::string s = "";

  // 0b typeid : Float64
  PUT_ID(pEnc, MY_FIELD_A, 3000444888.325);  s += "01000b0266660afbe45ae641";
  // 0a # typeid : Float32
  PUT_ID(pEnc, MY_FIELD_B, (float)123.456);         s += "01010a3679e9f642";
  enc.putRowSep();                           s += "03";

  PUT_ID(pEnc, MY_FIELD_A, 3000444888.325);  s += "8066660afbe45ae641";
  PUT_ID(pEnc, MY_FIELD_B, (float)123.456);  s += "8179e9f642";

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;

}

TEST_F(EncTest, encodesUsingFieldId)
{
  auto pEnc = crow::EncoderNew();
  auto &enc = *pEnc;

  std::string s = "";

  PUT_ID(pEnc, MY_FIELD_A, "Larry");  s += "01000102054c61727279";
  PUT_ID(pEnc, MY_FIELD_B, 23);       s += "010102362e";
  PUT_ID(pEnc, MY_FIELD_C, true);     s += "0102096601";
  enc.putRowSep();                    s += "03";

  PUT_ID(pEnc, MY_FIELD_A, "Moe");    s += "80034d6f65";
  PUT_ID(pEnc, MY_FIELD_B, 62);       s += "817c";
  PUT_ID(pEnc, MY_FIELD_C, false);    s += "8200";
  enc.putRowSep();                    s += "03";

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}


TEST_F(EncTest, encodesOutOfOrder)
{
  auto pEnc = crow::EncoderNew();
  auto &enc = *pEnc;

  std::string s = "";

  //first row sets index order
  // each use TFIELDINFO followed by value

  PUT_ID(pEnc, MY_FIELD_A, "Larry");  s += "01000102054c61727279";
  PUT_ID(pEnc, MY_FIELD_B, 23);       s += "010102362e";
  PUT_ID(pEnc, MY_FIELD_C, true);     s += "0102096601";
  enc.putRowSep();                    s += "03";

  PUT_ID(pEnc, MY_FIELD_C, false);    s += "8200";
  PUT_ID(pEnc, MY_FIELD_B, 62);       s += "817c";
  PUT_ID(pEnc, MY_FIELD_A, "Moe");    s += "80034d6f65";
  enc.putRowSep();                    s += "03";

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}

TEST_F(EncTest, encodesSparse)
{
  auto pEnc = crow::EncoderNew();
  auto &enc = *pEnc;

  std::string s = "";

  PUT_ID(pEnc, MY_FIELD_A, "Larry");  s += "01000102054c61727279";
  PUT_ID(pEnc, MY_FIELD_B, 23);       s += "010102362e";
  enc.putRowSep();                    s += "03";

  PUT_ID(pEnc, MY_FIELD_C, true);     s += "0102096601";
  enc.putRowSep();                    s += "03";

  PUT_ID(pEnc, MY_FIELD_A, "Moe");    s += "80034d6f65";
  enc.putRowSep();                    s += "03";

  PUT_ID(pEnc, MY_FIELD_B, 62);       s += "817c";
  PUT_ID(pEnc, MY_FIELD_C, false);    s += "8200";

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}

TEST_F(EncTest, encodesSet)
{
  auto pEnc = crow::EncoderNew();
  auto &enc = *pEnc;

  std::string s = "";

  PUT_ID(pEnc, MY_FIELD_A, "Larry");  s += "01000102054c61727279";

  pEnc->startSet();
  PUT_ID(pEnc, MY_FIELD_B, 23);       s += "11010236";
  PUT_ID(pEnc, MY_FIELD_C, true);     s += "11020966";
  auto setId = pEnc->endSet();       s += "040004812e8201";

  pEnc->putSetRef(setId);                s += "0500";
  enc.putRowSep();                    s += "03";

  PUT_ID(pEnc, MY_FIELD_A, "Moe");    s += "80034d6f65";
  pEnc->putSetRef(setId,0x03);                s += "3500";

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

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

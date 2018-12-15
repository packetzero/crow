#include <gtest/gtest.h>
#include "../include/crow.hpp"

#include "test_defs.hpp"

class EncTest : public ::testing::Test {
 protected:
  virtual void SetUp() {

  }
};

TEST_F(EncTest, usingFieldObjIndex) {
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  std::string s = "";

  const crow::SPField fname = enc.fieldFor(TSTRING, "name");
  const crow::SPField fage = enc.fieldFor(TINT32,"age");
  const crow::SPField factive = enc.fieldFor(TUINT8,"active");

  enc[fname] = "bob";         s += "43000100046e616d65";
  enc[fage] = 23;             s += "4301020003616765";
  enc[factive] = true;        s += "4302090006616374697665";

  // row data
  s += "05";
  s += "8003626f62";
  s += "812e";
  s += "8201";

  enc.startRow();                   s += "05";

  enc[fname] = "jerry";    s += "80056a65727279";
  enc[fage] = 58 ;        s += "8174";
  enc[factive] = false;   s += "8200";
  enc.startRow();                   s += "05";

  enc[fname] = "linda";   s += "80056c696e6461";
  enc[fage] = 33 ;        s += "8142";
  enc[factive] = true;    s += "8201";

  enc.flush();

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}

#ifdef NEVER
TEST_F(EncTest, withColumnNames) {
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  std::string s = "";

  // subscripts

  enc["name"] = "bob";         s += "43000100046e616d65";
  enc["age"] = 23;             s += "4301020003616765";
  enc["active"] = true;        s += "4302090006616374697665";

  // row data
  s += "05";
  s += "8003626f62";
  s += "812e";
  s += "8201";

  enc.startRow();                   s += "05";

  enc["name"] = "jerry";    s += "80056a65727279";
  enc["age"] = 58 ;        s += "8174";
  enc["active"] = false;   s += "8200";
  enc.startRow();                   s += "05";

  enc["name"] = "linda";   s += "80056c696e6461";
  enc["age"] = 33 ;        s += "8142";
  enc["active"] = true;    s += "8201";
  enc.flush();

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}
#endif // NEVER

TEST_F(EncTest, encodeFloats)
{
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  std::string s = "";

  const crow::SPField A = enc.fieldFor(TFLOAT64, MY_FIELD_A);
  const crow::SPField B = enc.fieldFor(TFLOAT32, MY_FIELD_B);

  // 0b typeid : Float64
  enc[A] = 3000444888.325;        s += "03000b02";  // def
  // 0a # typeid : Float32
  enc[B] = (float)123.456;        s += "03010a36"; // def

  // data
  s += "05";
  s += "8066660afbe45ae641";
  s += "8179e9f642";

  enc.startRow();                           s += "05";

  enc[A] = 3000444888.325;  s += "8066660afbe45ae641";
  enc[B] = (float)123.456;  s += "8179e9f642";

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;

}

TEST_F(EncTest, encodesUsingFieldId)
{
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  std::string s = "";

  const crow::SPField A = enc.fieldFor(TSTRING, MY_FIELD_A);
  const crow::SPField B = enc.fieldFor(TINT32, MY_FIELD_B);
  const crow::SPField C = enc.fieldFor(TUINT8, MY_FIELD_C);


  enc[A] = "Larry";  s += "03000102";
  enc[B] = 23;       s += "03010236";
  enc[C] = true;     s += "03020966";

  s += "05";
  s += "80054c61727279";
  s += "812e";
  s += "8201";

  enc.startRow();                    s += "05";
 // pEnc->put("Moe",MY_FIELD_A);        s += "80034d6f65";
  enc[B] = 62;       s += "817c";
  enc[C] = false;    s += "8200";

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}


TEST_F(EncTest, encodesOutOfOrder)
{
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  std::string s = "";

  const crow::SPField A = enc.fieldFor(TSTRING, MY_FIELD_A);
  const crow::SPField B = enc.fieldFor(TINT32, MY_FIELD_B);
  const crow::SPField C = enc.fieldFor(TUINT8, MY_FIELD_C);


  //first row sets index order
  // each use TFIELDINFO followed by value

  enc[A] = "Larry";  s += "03000102";
  enc[B] = 23;       s += "03010236";
  enc[C] = true;     s += "03020966";

  s += "05";
  s += "80054c61727279";
  s += "812e";
  s += "8201";

  enc.startRow();                    s += "05";

  enc[C] = false;    s += "8200";
  enc[B] = 62;       s += "817c";
  enc[A] = "Moe";    s += "80034d6f65";

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}

TEST_F(EncTest, encodesSparse)
{
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  const crow::SPField A = enc.fieldFor(TSTRING, MY_FIELD_A);
  const crow::SPField B = enc.fieldFor(TINT32, MY_FIELD_B);
  const crow::SPField C = enc.fieldFor(TUINT8, MY_FIELD_C);

  std::string s = "";

  enc[A] = "Larry";  s += "03000102";
  enc[B] = 23;       s += "03010236";

  s += "05"; // start data row
  s += "80054c61727279";
  s += "812e";

  enc.startRow();
  enc[C] = true;     s += "03020966";
  s += "05";
  s += "8201";

  enc.startRow();                    s += "05";

  enc[A] = "Moe";    s += "80034d6f65";
  enc.startRow();                    s += "05";

  enc[B] = 62;       s += "817c";
  enc[C] = false;    s += "8200";

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}


/*
TEST_F(EncTest, encodesSet)
{
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  std::string s = "";

  enc[MY_FIELD_A] = "Larry";  s += "01000102054c61727279";

  pEnc->startSet();
  enc[MY_FIELD_B] = 23;       s += "11010236";
  enc[MY_FIELD_C] = true;     s += "11020966";
  auto setId = pEnc->endSet();       s += "040004812e8201";

  pEnc->putSetRef(setId);                s += "0500";
  enc.startRow();                    s += "03";

  enc[MY_FIELD_A] = "Moe";    s += "80034d6f65";
  pEnc->putSetRef(setId,0x03);                s += "3500";

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}
*/
TEST_F(EncTest, encodesSubids)
{
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  const crow::SPField A = enc.fieldFor(TUINT32, 1);
  const crow::SPField B = enc.fieldFor(TUINT32, 1, 44000);

  std::string s = "";

  enc[A] = 0x22U;          s += "03000301";
  enc[B] = 0x4cU;   s += "23010301e0d702";
  s += "05"; // data row start
  s += "8022";
  s += "814c";

  enc.startRow();            s += "05";
  enc[A] = 0x3AU;          s += "803a";
  enc[B] = 0x1BU;   s += "811b";
  enc.flush();

  std::string actual;
  BytesToHexString(enc.data(), enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}

// this is based on withColumnNames test, but adding a couple of decorator fields
TEST_F(EncTest, decorators) {
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  std::string s = "";

  enc.startTable(TABLE_FLAG_DECORATE);   s += "12";

  const crow::SPField DATE = enc.fieldFor(TSTRING, "date");
  const crow::SPField DOMAIN = enc.fieldFor(TINT32, "domain");

  enc[DATE] = "20180502";         s += "430001000464617465";
  enc[DOMAIN] = 23;               s += "4301020006646f6d61696e";

  s += "05";
  s += "80083230313830353032";
  s += "812e";

  enc.startTable();            s += "02";

  const crow::SPField NAME = enc.fieldFor(TSTRING, "name");
  const crow::SPField AGE = enc.fieldFor(TINT32, "age");
  const crow::SPField ACTIVE = enc.fieldFor(TUINT8, "active");

  enc[NAME] = "bob";         s += "43000100046e616d65";
  enc[AGE] = 23;             s += "4301020003616765";
  enc[ACTIVE] = true;        s += "4302090006616374697665";

  // row data
  s += "05";
  s += "8003626f62";
  s += "812e";
  s += "8201";

  enc.startRow();                   s += "05";

  enc[NAME] = "jerry";    s += "80056a65727279";
  enc[AGE] = 58 ;        s += "8174";
  enc[ACTIVE] = false;   s += "8200";
  enc.startRow();                   s += "05";

  enc[NAME] = "linda";   s += "80056c696e6461";
  enc[AGE] = 33 ;        s += "8142";
  enc[ACTIVE] = true;    s += "8201";
  enc.flush();

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

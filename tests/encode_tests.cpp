#include <gtest/gtest.h>
#include "../include/crow.hpp"

#include "test_defs.hpp"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int status= RUN_ALL_TESTS();
  return status;
}

class EncTest : public ::testing::Test {
 protected:
  virtual void SetUp() {

  }
};

static const SPFieldDef fname = FieldDef::alloc(TSTRING, "name");
static const SPFieldDef fage = FieldDef::alloc(TINT32, "age");
static const SPFieldDef factive = FieldDef::alloc(TUINT8, "active");

static const SPFieldDef A = FieldDef::alloc(TSTRING, MY_FIELD_A);
static const SPFieldDef B = FieldDef::alloc(TINT32, MY_FIELD_B);
static const SPFieldDef C = FieldDef::alloc(TUINT8, MY_FIELD_C);

static const SPFieldDef D = FieldDef::alloc(TFLOAT64, MY_FIELD_A);
static const SPFieldDef F = FieldDef::alloc(TFLOAT32, MY_FIELD_B);


TEST_F(EncTest, usingFieldObjIndex) {
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  std::string s = "";

  enc.put(fname, "bob");     s += "43000100046e616d65";
  enc.put(fage, 23);         s += "4301020003616765";
  enc.put(factive, true);    s += "4302090006616374697665";

  // row data
  s += "05";
  s += "8003626f62";
  s += "812e";
  s += "8201";

  enc.startRow();                   s += "05";

  enc.put(fname, "jerry");   s += "80056a65727279";
  enc.put(fage, 58);         s += "8174";
  enc.put(factive, false);   s += "8200";
  enc.startRow();                   s += "05";

  enc.put(fname, "linda");   s += "80056c696e6461";
  enc.put(fage, 33);         s += "8142";
  enc.put(factive, true);    s += "8201";

  enc.flush();

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}


TEST_F(EncTest, encodeFloats)
{
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  std::string s = "";


  // 0b typeid : Float64
  enc.put(D, 3000444888.325);        s += "03000b02";  // def
  // 0a # typeid : Float32
  enc.put(F, (float)123.456);        s += "03010a36"; // def

  // data
  s += "05";
  s += "8066660afbe45ae641";
  s += "8179e9f642";

  enc.startRow();                           s += "05";

  enc.put(D, 3000444888.325);  s += "8066660afbe45ae641";
  enc.put(F, (float)123.456);  s += "8179e9f642";

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

  enc.put(A, "Larry");  s += "03000102";
  enc.put(B, 23);       s += "03010236";
  enc.put(C, true);     s += "03020966";

  s += "05";
  s += "80054c61727279";
  s += "812e";
  s += "8201";

  enc.startRow();                    s += "05";
 // pEnc->put("Moe",MY_FIELD_A);        s += "80034d6f65";
  enc.put(B, 62);       s += "817c";
  enc.put(C, false);    s += "8200";

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

  //first row sets index order
  // each use TFIELDINFO followed by value

  enc.put(A, "Larry");  s += "03000102";
  enc.put(B, 23);       s += "03010236";
  enc.put(C, true);     s += "03020966";

  s += "05";
  s += "80054c61727279";
  s += "812e";
  s += "8201";

  enc.startRow();                    s += "05";

  enc.put(C, false);    s += "8200";
  enc.put(B, 62);       s += "817c";
  enc.put(A, "Moe");    s += "80034d6f65";

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

  std::string s = "";

  enc.put(A, "Larry");  s += "03000102";
  enc.put(B, 23);       s += "03010236";

  s += "05"; // start data row
  s += "80054c61727279";
  s += "812e";

  enc.startRow();
  enc.put(C, true);     s += "03020966";
  s += "05";
  s += "8201";

  enc.startRow();                    s += "05";

  enc.put(A, "Moe");    s += "80034d6f65";
  enc.startRow();                    s += "05";

  enc.put(B, 62);       s += "817c";
  enc.put(C, false);    s += "8200";

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}

// Invalid DynVal with no type set are like null values
// The field headers should be written, but not the data.
TEST_F(EncTest, nullValues) {
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  std::string s = "";

  DynVal nullValue;

  enc.put(fname, nullValue);     s += "43000100046e616d65";
  enc.put(fage, nullValue);         s += "4301020003616765";
  enc.put(factive, true);    s += "4302090006616374697665";

  // row data
  s += "05";
//  s += "8003626f62";
//  s += "812e";
  s += "8201";

  enc.startRow();                   s += "05";

  enc.put(fname, "jerry");   s += "80056a65727279";
  enc.put(fage, 58);         s += "8174";
  enc.put(factive, false);   s += "8200";
  enc.startRow();                   s += "05";

  enc.put(fname, "linda");   s += "80056c696e6461";
  enc.put(fage, 33);         s += "8142";
  enc.put(factive, true);    s += "8201";

  enc.flush();

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}

TEST_F(EncTest, trackRowSizes) {
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;
  
  std::string s = "";
  
  DynVal nullValue;
  
  enc.put(fname, "bob");     s += "43000100046e616d65";
  enc.put(fage, nullValue);         s += "4301020003616765";
  enc.put(factive, true);    s += "4302090006616374697665";
  
  // flush only headers

  enc.flush(true);
  EXPECT_EQ(s.size()/2, enc.size());
  size_t headerSize = enc.size();
  size_t lastSize = enc.size();

  // row data
  s += "05";
  s += "8003626f62";
  //  s += "812e";
  s += "8201";

  enc.flush();
  size_t rowSize = enc.size() - headerSize;
  EXPECT_EQ(8, rowSize);
  lastSize = enc.size();


  enc.startRow();                   s += "05";
  
  enc.put(fname, "jerry");   s += "80056a65727279";
  enc.put(fage, 58);         s += "8174";
  enc.put(factive, false);   s += "8200";

  enc.flush();
  rowSize = enc.size() - lastSize;
  EXPECT_EQ(12, rowSize);
  lastSize = enc.size();

  enc.startRow();                   s += "05";
  enc.put(fname, "linda");   s += "80056c696e6461";
  enc.put(fage, 33);         s += "8142";
  enc.put(factive, true);    s += "8201";
  
  enc.flush();
  

  rowSize = enc.size() - lastSize;
  EXPECT_EQ(12, rowSize);

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

  // overrides the global scoped A, B
  const SPFieldDef A = FieldDef::alloc(TUINT32, 1);
  const SPFieldDef B = FieldDef::alloc(TUINT32, 1, 44000);

  std::string s = "";

  enc.put(A, 0x22U);          s += "03000301";
  enc.put(B, 0x4cU);   s += "23010301e0d702";
  s += "05"; // data row start
  s += "8022";
  s += "814c";

  enc.startRow();            s += "05";
  enc.put(A, 0x3AU);          s += "803a";
  enc.put(B, 0x1BU);   s += "811b";
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

  const SPFieldDef DATE = FieldDef::alloc(TSTRING, "date");
  const SPFieldDef DOMA = FieldDef::alloc(TINT32, "domain");

  enc.put(DATE, "20180502");         s += "430001000464617465";
  enc.put(DOMA, 23);               s += "4301020006646f6d61696e";

  s += "05";
  s += "80083230313830353032";
  s += "812e";

  enc.startTable();            s += "02";

  enc.put(fname, "bob");         s += "43000100046e616d65";
  enc.put(fage, 23);             s += "4301020003616765";
  enc.put(factive, true);        s += "4302090006616374697665";

  // row data
  s += "05";
  s += "8003626f62";
  s += "812e";
  s += "8201";

  enc.startRow();                   s += "05";

  enc.put(fname, "jerry");    s += "80056a65727279";
  enc.put(fage, 58);        s += "8174";
  enc.put(factive, false);   s += "8200";
  enc.startRow();                   s += "05";

  enc.put(fname, "linda");   s += "80056c696e6461";
  enc.put(fage, 33 );        s += "8142";
  enc.put(factive, true);    s += "8201";
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

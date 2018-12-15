#include <gtest/gtest.h>
#include "../include/crow.hpp"
#include "test_defs.hpp"

class EncStructTest : public ::testing::Test {
 protected:
  virtual void SetUp() {

  }
};

TEST_F(EncStructTest, encodesStructAndVariable)
{
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  std::string s = "";
  Person person = Person();

  int fieldId = 10;
  enc.struct_hdr(crow::type_for(person.age), fieldId++);
  enc.struct_hdr(crow::type_for(person.active), fieldId++);
  enc.struct_hdr(crow::type_for(person.name), fieldId++, 0, "", sizeof(person.name));

  s += "1300020a";
  s += "1301090b";
  s += "1302010c03";

  PERSON(person,"Bob", 23, true);
  enc.put_struct(&person, sizeof(person));

  const crow::SPField NAME = enc.fieldFor(TSTRING, "name");


  enc[NAME] = "bo";         s += "43030100046e616d65";  // field def

  // row data

  s += "05";
  s += "17000000";
  s += "01";
  s += "426f62";

  s += "04"; // length of variable field section
  s += "8302626f";

  PERSON(person,"Moe", 62, false);
  enc.startRow();
  enc.put_struct(&person, sizeof(person));

  enc[NAME] = "bobo";

  s += "05";
  s += "3e000000";
  s += "00";
  s += "4d6f65";

  s += "06"; // length of variable field section
  s += "8304626f626f";

  // third row : test no variable data
  enc.startRow();
  enc.put_struct(&person, sizeof(person));

  s += "05";
  s += "3e000000";
  s += "00";
  s += "4d6f65";

  s += "00"; // length of variable field section

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}

TEST_F(EncStructTest, encStructFirst)
{
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  const crow::SPField NAME = enc.fieldFor(TSTRING, "name");

  Person person = Person();

  int fieldId = 10;
  try {
    enc.struct_hdr(crow::type_for(person.age), fieldId++);
    enc[NAME] = "bob";

    // The following should throw exception.
    // struct columns need to come before variable

    enc.struct_hdr(crow::type_for(person.active), fieldId++);
    FAIL();
  } catch (...) {
    ASSERT_TRUE(true);
  }
}

TEST_F(EncStructTest, invalidStructSize)
{
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  Person person = Person();

  int fieldId = 10;
  try {
    enc.struct_hdr(crow::type_for(person.age), fieldId++);

    // The following should throw exception.
    // size of struct does not match def

    enc.put_struct(&person, 3);
    FAIL();
  } catch (...) {
    ASSERT_TRUE(true);
  }
}

TEST_F(EncStructTest, encodesStruct)
{
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  std::string s = "";
  Person person = Person();

  int fieldId = 10;
  enc.struct_hdr(crow::type_for(person.age), fieldId++);
  enc.struct_hdr(crow::type_for(person.active), fieldId++);
  enc.struct_hdr(crow::type_for(person.name), fieldId++, 0, "", sizeof(person.name));

  s += "1300020a";
  s += "1301090b";
  s += "1302010c03";

  PERSON(person,"Bob", 23, true);
  enc.put_struct(&person, sizeof(person));

  s += "05";
  s += "17000000";
  s += "01";
  s += "426f62";

  PERSON(person,"Moe", 62, false);
  enc.startRow();
  enc.put_struct(&person, sizeof(person));

  s += "05";
  s += "3e000000";
  s += "00";
  s += "4d6f65";

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}

TEST_F(EncStructTest, encodesStructEnd)
{
  auto pEnc = crow::EncoderFactory::New();
  auto &enc = *pEnc;

  std::string s = "";
  Person person = Person();

  enc.struct_hdr(crow::type_for(person.age), "_D");
  enc.struct_hdr(crow::type_for(person.active), "_V");
  enc.struct_hdr(crow::type_for(person.name), "_T", sizeof(person.name));

  enc.startRow();

  s += "53000200025f44";
  s += "53010900025f56";
  s += "53020100025f5403";

  const crow::SPField NAME = enc.fieldFor(TSTRING, "name");
  
  enc[NAME] = "bo";         s += "43030100046e616d65";  // field def

  PERSON(person,"Bob", 23, true);
  enc.put_struct(&person, sizeof(person));

  // row data

  s += "05";
  s += "17000000";
  s += "01";
  s += "426f62";

  s += "04"; // length of variable field section
  s += "8302626f";

  const uint8_t* result = enc.data();

  std::string actual;
  BytesToHexString(result, enc.size(), actual);

  if (ENC_GTEST_LOG_ENABLED) printf(" %s\n", actual.c_str());

  ASSERT_EQ(s, actual);

  delete pEnc;
}

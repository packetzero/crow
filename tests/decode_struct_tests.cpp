#include <gtest/gtest.h>
#include "../include/crow.hpp"
#include "test_defs.hpp"

class DecStructTest : public ::testing::Test {
 protected:
  virtual void SetUp() {

  }
};

TEST_F(DecStructTest, decodeStruct)
{
  auto vec = std::vector<uint8_t>();
  HexStringToVec("1300020a1301090b1302010c03051700000001426f62053e000000004d6f65", vec);

  auto dl = crow::GenericDecoderListener();
  auto pDec = crow::DecoderNew(vec.data(), vec.size());
  auto &dec = *pDec;
  auto status = dec.decode(dl);
  ASSERT_EQ(2, status);
  std::string actual = to_csv(dl._rows, dl._structData, dec.getFields());
  std::string headers = to_header_csv(pDec->getFields());

  ASSERT_EQ("23,1,Bob||62,0,Moe||", actual);
  ASSERT_EQ("10,11,12", headers);

  delete pDec;
}

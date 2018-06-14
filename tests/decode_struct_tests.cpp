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
  auto numRows = dec.decode(dl);
  ASSERT_EQ(2, numRows);
  std::string actual = to_csv(dl._rows, dl._structData, dec.getFields());
  std::string headers = to_header_csv(pDec->getFields());

  ASSERT_EQ("23,1,Bob||62,0,Moe||", actual);
  ASSERT_EQ("10,11,12", headers);

  delete pDec;
}

TEST_F(DecStructTest, decodeStructAndVariable)
{
  auto vec = std::vector<uint8_t>();
  HexStringToVec("1300020a1301090b1302010c0343030100046e616d65051700000001426f62048302626f053e000000004d6f65068304626f626f053e000000004d6f6500", vec);

  auto dl = crow::GenericDecoderListener();
  auto pDec = crow::DecoderNew(vec.data(), vec.size());
  auto &dec = *pDec;
  auto numRows = dec.decode(dl);
  ASSERT_EQ(3, numRows);
  std::string actual = to_csv(dl._rows, dl._structData, dec.getFields());
  std::string headers = to_header_csv(pDec->getFields());

  ASSERT_EQ("23,1,Bob,bo||62,0,Moe,bobo||62,0,Moe||", actual);
  ASSERT_EQ("10,11,12,name", headers);

  delete pDec;
}

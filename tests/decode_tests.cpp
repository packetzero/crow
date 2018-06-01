#include <gtest/gtest.h>
#include "../include/crow.hpp"


int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  int status= RUN_ALL_TESTS();
  return status;
}

void HexStringToVec(const std::string str, Bytes &dest);

class DecTest : public ::testing::Test {
 protected:
  virtual void SetUp() {

  }
};

bool needs_quotes(std::string s)
{
  if (s.find(",") != std::string::npos) { return true; }
  if (s.find('"') != std::string::npos) { return true; }
  // TODO: escape quotes like CSV
  return false;
}

std::string quoted(std::string str)
{
  int numQuotes = 0;
  int numCommas = 0;
  const char *p=str.c_str();
  const char *end = p + str.length();
  while (p < end) {
    if (*p == ',') { numCommas++; }
    if (*p == '"') { numQuotes++; }
    p++;
  }

  if (numCommas == 0 && numQuotes == 0) return str;
  if (numQuotes == 0) return "\"" + str + "\"";

  p=str.c_str();
  std::string dest(str.length() + numQuotes + 2, ' ');
  char *d = (char *)dest.c_str();
  *d++ = '"';
  while (p < end) {
    char c = *p++;
    *d++ = c;
    if (c == '"') {
      *d++ = c;
    }
  }
  *d++ = '"';

  return dest;
}

void BytesToHexString(const std::string &bytes, std::string &dest);

std::string to_header_csv(std::vector<crow::Field> fields)
{
  static char tmp[48];
  std::string s;
  for (auto &fld : fields) {
    if (fld.index > 0) s += ",";
    if (fld.id > 0) {
      sprintf(tmp,"%d", fld.id);
      s += tmp;
      if (fld.subid > 0) {
        sprintf(tmp,"_%d", fld.subid);
        s += tmp;
      }
    }
    if (fld.name.length() > 0) { s += fld.name; };
  }
  return s;
}

std::string to_csv(std::vector<crow::GenDecRow> &rows)
{
  static char tmp[48];
  std::string s;
  for (auto &row : rows) {
    for (auto it = row.begin(); it != row.end(); it++) {
      auto &col = it->second;
      if (col.fieldIndex > 0) s += ",";
      if (col.typeId == CrowType::TSTRING && needs_quotes(col.strval)) {
        s += quoted(col.strval);
      } else if (col.typeId == CrowType::TBYTES) {
        std::string hex;
        BytesToHexString(col.strval, hex);
        s += hex;
      } else {
        s += col.strval;
      }
      if (col.flags > 0) {
        sprintf(tmp," FLAGS:%x", col.flags);
        s += tmp;
      }
      //sprintf(tmp, " (type:%d)", col.typeId);
      //s += tmp;
    }
    s += "||";
  }
  return s;
}
/*
std::string _typeName(CrowType ft) {
  switch(ft) {
    case TINT32: return "int32";
    case TUINT32: return "uint32";
    case TINT64: return "int64";
    case TUINT64: return "uint64";
    case TFLOAT64: return "double";
    default:
      break;
  }
  return "";
}
*/

TEST_F(DecTest, decodesUsingFieldNames) {
  auto vec = std::vector<uint8_t>();
  HexStringToVec("43000100046e616d6543010200036167654302090006616374697665058003626f62812e82010580056a65727279817482000580056c696e646181428201", vec);

  auto dl = crow::GenericDecoderListener();
  auto pDec = crow::DecoderNew(vec.data(), vec.size());
  auto &dec = *pDec;
  dec.decode(dl);
  std::string actual = to_csv(dl._rows);

  ASSERT_EQ("bob,23,1||jerry,58,0||linda,33,1||", actual);

  delete pDec;
}

TEST_F(DecTest, decodesFloats) {
  auto vec = std::vector<uint8_t>();
  HexStringToVec("03000b0203010a36058066660afbe45ae6418179e9f642058066660afbe45ae6418179e9f642", vec);

  auto dl = crow::GenericDecoderListener();
  auto pDec = crow::DecoderNew(vec.data(), vec.size());
  auto &dec = *pDec;
  dec.decode(dl);
  std::string actual = to_csv(dl._rows);

  ASSERT_EQ("3000444888.325,123.456||3000444888.325,123.456||", actual);

  delete pDec;
}

TEST_F(DecTest, decodesUsingFieldId) {
  auto vec = std::vector<uint8_t>();
  HexStringToVec("0300010203010236030209660580054c61727279812e82010580034d6f65817c8200", vec);

  auto dl = crow::GenericDecoderListener();
  auto pDec = crow::DecoderNew(vec.data(), vec.size());
  auto &dec = *pDec;
  dec.decode(dl);
  std::string actual = to_csv(dl._rows);

  ASSERT_EQ("Larry,23,1||Moe,62,0||", actual);

  delete pDec;
}

TEST_F(DecTest, decodesBytes) {
  auto vec = std::vector<uint8_t>();
  HexStringToVec("03000c020580040badcafe0580040badcafe", vec);

  auto dl = crow::GenericDecoderListener();
  auto pDec = crow::DecoderNew(vec.data(), vec.size());
  auto &dec = *pDec;
  dec.decode(dl);
  std::string actual = to_csv(dl._rows);

  ASSERT_EQ("0badcafe||0badcafe||", actual);

  delete pDec;
}

TEST_F(DecTest, decodesFlags) {
  auto vec = std::vector<uint8_t>();
  HexStringToVec("0300030205800305178004058005", vec);

  auto dl = crow::GenericDecoderListener();
  auto pDec = crow::DecoderNew(vec.data(), vec.size());
  auto &dec = *pDec;
  dec.decode(dl);
  std::string actual = to_csv(dl._rows);

  ASSERT_EQ("3||4 FLAGS:1||5||", actual);

  delete pDec;
}
/*
TEST_F(DecTest, decodesSet) {
  auto vec = std::vector<uint8_t>();
  HexStringToVec("01000102054c617272791101023611020966040004812e820105000380034d6f653500", vec);

  auto dl = crow::GenericDecoderListener();
  auto pDec = crow::DecoderNew(vec.data(), vec.size());
  auto &dec = *pDec;
  dec.decode(dl);
  std::string actual = to_csv(dl._rows);

  ASSERT_EQ("Larry,23,1||Moe,23,1||", actual);

  delete pDec;
}
*/

TEST_F(DecTest, decodeSubids) {
  auto vec = std::vector<uint8_t>();
  HexStringToVec("0300030123010301e0d702058022814c05803a811b", vec);

  auto dl = crow::GenericDecoderListener();
  auto pDec = crow::DecoderNew(vec.data(), vec.size());
  auto &dec = *pDec;
  dec.decode(dl);
  std::string actual = to_csv(dl._rows);
  std::string headers = to_header_csv(pDec->getFields());

  ASSERT_EQ("34,76||58,27||", actual);
  ASSERT_EQ("1,1_44000", headers);

  delete pDec;
}

TEST_F(DecTest, empty) {
  auto vec = std::vector<uint8_t>();

  auto dl = crow::GenericDecoderListener();
  auto pDec = crow::DecoderNew(vec.data(), vec.size());
  pDec->decode(dl);

  ASSERT_EQ(0, dl._rows.size());

  delete pDec;
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

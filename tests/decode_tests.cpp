#include <gtest/gtest.h>
#include "../include/crow.hpp"
#include "../include/crow/crow_test_decoder.hpp"
#include "test_defs.hpp"



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

std::vector< crow::SPCFieldInfo> index_sorted_map(crow::GenDecRow &obj)
{
  auto indexMap = std::vector< crow::SPCFieldInfo>(100);
  for (auto it = obj.begin(); it != obj.end(); it++) {
    crow::SPCFieldInfo field = it->first;
    indexMap[(int)field->index] = field;
  }
  return indexMap;
}


std::string to_header_csv(std::vector<crow::SPCFieldInfo> fields, crow::GenDecRow *decoratorFields)
{
  static char tmp[48];
  std::string s;
  int i=-1;
  for (auto &fld : fields) {
    i++;
    if (i > 0) s += ",";
    if (fld->id > 0) {
      sprintf(tmp,"%d", fld->id);
      s += tmp;
      if (fld->schemaId > 0) {
        sprintf(tmp,"_%d", fld->schemaId);
        s += tmp;
      }
    }
    if (fld->name.length() > 0) { s += fld->name; };
  }
  if (decoratorFields != nullptr) {
    auto indexMap = index_sorted_map(*decoratorFields);

    for ( int i=0; i < indexMap.size() ; i++) {
      auto field = indexMap[i];
      if (!field) { continue; }

      s += ",";
      s += field->name;
    }
  }
  return s;
}

/*
 * Render a value from C struct
 */
void renderStructValue(std::string &s, uint8_t* &p, crow::SPCFieldInfo field)//, crow::DecColValue &value)
{
  char tmp[64];
  switch(field->typeId) {
    case CrowType::TSTRING: {
      std::string val = std::string(p, p + field->structFieldLength);
      s += val;
      p += field->structFieldLength;
      break;
    }
    case CrowType::TBYTES: {
      std::string val;
      BytesToHexString(p, field->structFieldLength, val);
      s += val;
      p += field->structFieldLength;
      break;
    }
    case CrowType::TINT8:
      snprintf(tmp, sizeof(tmp), "%d", *((int8_t*)p) );
      s += tmp;
      p += 1;
      break;
    case CrowType::TINT16:
      snprintf(tmp, sizeof(tmp), "%d", *((int16_t*)p) );
      s += tmp;
      p += 2;
      break;
    case CrowType::TINT32:
      snprintf(tmp, sizeof(tmp), "%d", *((int32_t*)p) );
      s += tmp;
      p += 4;
      break;
    case CrowType::TINT64:
      snprintf(tmp, sizeof(tmp), "%lld", *((int64_t*)p) );
      s += tmp;
      p += 8;
      break;
    case CrowType::TUINT8:
      snprintf(tmp, sizeof(tmp), "%u", *((uint8_t*)p) );
      s += tmp;
      p += 1;
      break;
    case CrowType::TUINT16:
      snprintf(tmp, sizeof(tmp), "%u", *((uint16_t*)p) );
      s += tmp;
      p += 2;
      break;
    case CrowType::TUINT32:
      snprintf(tmp, sizeof(tmp), "%u", *((uint32_t*)p) );
      s += tmp;
      p += 4;
      break;
    case CrowType::TUINT64:
      snprintf(tmp, sizeof(tmp), "%llu", *((uint64_t*)p) );
      s += tmp;
      p += 8;
      break;
    case CrowType::TFLOAT32:
      snprintf(tmp, sizeof(tmp), "%.3f", *((float*)p) );
      s += tmp;
      p += 4;
      break;
    case CrowType::TFLOAT64:
      snprintf(tmp, sizeof(tmp), "%.3f", *((double*)p) );
      s += tmp;
      p += 8;
      break;

    default: {
      throw new std::runtime_error("unknown type");
    }
  }

}


static void renderFieldValue(crow::SPCFieldInfo field, crow::DecColValue &col, std::string &s)
{
  static char tmp[48];
  if (field->typeId == CrowType::TSTRING && needs_quotes(col.as_s())) {
    s += quoted(col.as_s());
  } else if (field->typeId == CrowType::TBYTES) {
    std::string hex;
    BytesToHexString(col.as_s(), hex);
    s += hex;
  } else {
    s += col.as_s();
  }
  if (col.flags > 0) {
    sprintf(tmp," FLAGS:%x", col.flags);
    s += tmp;
  }
}

void row_to_csv(crow::GenDecRow &row, std::string &s)
{
  auto indexMap = index_sorted_map(row);
  
  for ( int i=0; i < indexMap.size() ; i++) {
    auto field = indexMap[i];
    if (!field) { continue; }
    crow::DecColValue &col = row[field];
    if (s.size() > 0) s += ",";
    renderFieldValue(field, col, s);
  }
}

std::string to_csv(std::vector<crow::GenDecRow> &rows, crow::GenDecRow *decoratorFields)
{
  std::string s;
  for (auto &row : rows) {
    
    std::string line = "";
    row_to_csv(row, line);
/*
    auto indexMap = index_sorted_map(row);

    for ( int i=0; i < indexMap.size() ; i++) {
      auto field = indexMap[i];
      if (!field) { continue; }
      crow::DecColValue &col = row[field];
      if (field->index > 0) s += ",";

      renderFieldValue(field, col, s);
    }
 */
    if (decoratorFields != nullptr) {
      row_to_csv(*decoratorFields, line);
      /*
      auto indexMap = index_sorted_map(*decoratorFields);
      for ( int i=0; i < indexMap.size() ; i++) {
        auto field = indexMap[i];
        if (!field) { continue; }
        s += ",";
        renderFieldValue(field, (*decoratorFields)[field], s);
      }
       */
    }
    s += line;
    s += "||";
  }
  return s;
}


std::string to_csv(std::vector<crow::GenDecRow> &rows,
    std::vector< std::vector<uint8_t> > &rowStructs,
                   std::vector<crow::SPCFieldInfo> fields)
{
  static char tmp[48];
  std::string s;
  auto structRow = rowStructs.begin();
  auto row = rows.begin();
  while(true) {
    int numProcessed = 0;
    std::string line = "";

    if (structRow != rowStructs.end()) {
      numProcessed++;
      auto p = structRow->data();
      auto end = p + structRow->size();
      int i=-1;
      for (auto &field : fields) {
        if (!field->isStructField()) break;
        if (p >= end) break;
        i++;
        if (i > 0) line += ",";

        renderStructValue(line, p, field);
      }
      structRow++;
    }

    // TODO: clean this mess up. no guarantee variable row data with structs

    if (row != rows.end()) {
      numProcessed++;

      row_to_csv(*row, line);

      row++;
    }

    s += line;

    if (numProcessed == 0) { break; }
    s += "||";
  }
  return s;
}


TEST_F(DecTest, decodesUsingFieldNames) {
  auto vec = std::vector<uint8_t>();
  HexStringToVec("43000100046e616d6543010200036167654302090006616374697665058003626f62812e82010580056a65727279817482000580056c696e646181428201", vec);

  auto dl = crow::GenericDecoderListener();
  auto pDec = crow::DecoderFactory::New(vec.data(), vec.size());
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
  auto pDec = crow::DecoderFactory::New(vec.data(), vec.size());
  auto &dec = *pDec;
  dec.decode(dl);
  std::string actual = to_csv(dl._rows);

  ASSERT_EQ("3000444888.325000,123.456001||3000444888.325000,123.456001||", actual);

  delete pDec;
}

TEST_F(DecTest, decodesUsingFieldId) {
  auto vec = std::vector<uint8_t>();
  HexStringToVec("0300010203010236030209660580054c61727279812e82010580034d6f65817c8200", vec);

  auto dl = crow::GenericDecoderListener();
  auto pDec = crow::DecoderFactory::New(vec.data(), vec.size());
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
  auto pDec = crow::DecoderFactory::New(vec.data(), vec.size());
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
  auto pDec = crow::DecoderFactory::New(vec.data(), vec.size());
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
  auto pDec = crow::DecoderFactory::New(vec.data(), vec.size());
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
  auto pDec = crow::DecoderFactory::New(vec.data(), vec.size());
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
  auto pDec = crow::DecoderFactory::New(vec.data(), vec.size());
  pDec->decode(dl);

  ASSERT_EQ(0, dl._rows.size());

  delete pDec;
}

TEST_F(DecTest, decorators) {
  auto vec = std::vector<uint8_t>();
  HexStringToVec("124300010004646174654301020006646f6d61696e0580083230313830353032812e0243000100046e616d6543010200036167654302090006616374697665058003626f62812e82010580056a65727279817482000580056c696e646181428201", vec);

  auto dl = crow::GenericDecoderListener();
  auto pDec = crow::DecoderFactory::New(vec.data(), vec.size());
  auto &dec = *pDec;
  dec.decode(dl);
  std::string actual = to_csv(dl._rows, &dl._decoratorFields);
  std::string headers = to_header_csv(pDec->getFields(), &dl._decoratorFields);

  ASSERT_EQ("bob,23,1,20180502,23||jerry,58,0,20180502,23||linda,33,1,20180502,23||", actual);
  ASSERT_EQ("name,age,active,date,domain", headers);

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

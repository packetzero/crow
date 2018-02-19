#ifndef _CROW_HPP_
#define _CROW_HPP_

#include <stdint.h>
#include <string>
#include <vector>

enum FieldType {
  TYPE_INT32          = 1,
  TYPE_UINT32         = 2,
  TYPE_INT64          = 3,
  TYPE_UINT64         = 4,
  TYPE_DOUBLE         = 5,
  TYPE_BOOL           = 6,
  TYPE_STRING         = 7,
  TYPE_BYTES          = 8,
  TYPE_ROWSEP         = 9,
  MAX_FIELD_TYPE      = 10,
};

typedef std::vector<uint8_t> Bytes;

namespace crow {

  class Encoder {
  public:
    virtual void put(uint32_t fieldIndex, int32_t value, const std::string name = "") = 0;
    virtual void put(uint32_t fieldIndex, uint32_t value, const std::string name = "") = 0;
    virtual void put(uint32_t fieldIndex, int64_t value, const std::string name = "") = 0;
    virtual void put(uint32_t fieldIndex, uint64_t value, const std::string name = "") = 0;
    virtual void put(uint32_t fieldIndex, double value, const std::string name = "") = 0;
    virtual void put(uint32_t fieldIndex, const std::string value, const std::string name = "") = 0;
    virtual void put(uint32_t fieldIndex, const char* str, const std::string name = "") = 0;
    virtual void put(uint32_t fieldIndex, const std::vector<uint8_t>& value, const std::string name = "") = 0;
    virtual void put(uint32_t fieldIndex, const uint8_t* bytes, size_t len, const std::string name = "") = 0;
    virtual void put(uint32_t fieldIndex, bool value, const std::string name = "") = 0;

    virtual void putRowSep() = 0;

    virtual const uint8_t* data() const = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;

    virtual ~Encoder() {}
  };

  class DecoderListener {
  public:
    virtual void onField(uint32_t fieldIndex, int32_t value, const std::string name_optional="") {}
    virtual void onField(uint32_t fieldIndex, uint32_t value, const std::string name_optional="") {}
    virtual void onField(uint32_t fieldIndex, int64_t value, const std::string name_optional="") {}
    virtual void onField(uint32_t fieldIndex, uint64_t value, const std::string name_optional="") {}
    virtual void onField(uint32_t fieldIndex, double value, const std::string name_optional="") {}
    virtual void onField(uint32_t fieldIndex, const std::string &value, const std::string name_optional="") {}
    virtual void onField(uint32_t fieldIndex, const std::vector<uint8_t> value, const std::string name_optional="") {}
    virtual void onRowSep() {}
  };

  class Decoder {
  public:
    virtual bool decode(const Bytes &encodedSrc, DecoderListener &listener) = 0;
    virtual bool decode(const uint8_t* pEncData, size_t encLength, DecoderListener &listener) = 0;
    virtual ~Decoder() {}
  };

  Encoder* EncoderNew(size_t initialCapacity = 4096);
  Decoder* DecoderNew();


  class DecoderLogger : public DecoderListener {
  public:
    DecoderLogger() : s() {}
    virtual void onField(uint32_t fieldIndex, int32_t value, const std::string name_optional) {
      char tmp[128];
      sprintf(tmp,"[%2d]%s int32 %d,",fieldIndex, (name_optional.length() > 0 ? name_optional.c_str() : ""), value);
      s += std::string(tmp);
     }
    virtual void onField(uint32_t fieldIndex, uint32_t value, const std::string name_optional) {
      char tmp[128];
      sprintf(tmp,"[%2d]%s uint32 %u,", fieldIndex, (name_optional.length() > 0 ? name_optional.c_str() : ""), value);
      s += std::string(tmp);
    }
    virtual void onField(uint32_t fieldIndex, int64_t value, const std::string name_optional) {
      char tmp[128];
      sprintf(tmp,"[%2d]%s int64 %lld,", fieldIndex, (name_optional.length() > 0 ? name_optional.c_str() : ""), value);
      s += std::string(tmp);
    }
    virtual void onField(uint32_t fieldIndex, uint64_t value, const std::string name_optional) {
      char tmp[128];
      sprintf(tmp,"[%2d]%s uint64 %llu,", fieldIndex, (name_optional.length() > 0 ? name_optional.c_str() : ""), value);
      s += std::string(tmp);
    }
    virtual void onField(uint32_t fieldIndex, double value, const std::string name_optional) {
      char tmp[128];
      sprintf(tmp,"[%2d]%s double %.3f,", fieldIndex, (name_optional.length() > 0 ? name_optional.c_str() : ""), value);
      s += std::string(tmp);
    }
    virtual void onField(uint32_t fieldIndex, const std::string &value, const std::string name_optional) {
      char tmp[128];
      sprintf(tmp,"[%2d]%s \"", fieldIndex, (name_optional.length() > 0 ? name_optional.c_str() : ""));
      s += std::string(tmp);
      s += value;
      s += "\",";
    }
    virtual void onField(uint32_t fieldIndex, const std::vector<uint8_t> value, const std::string name_optional) {
      char tmp[128];
      sprintf(tmp,"[%2d]%s bytes [%lu],", fieldIndex, (name_optional.length() > 0 ? name_optional.c_str() : ""), value.size());
      s += std::string(tmp);
    }

    std::string str() { return s; }
  private:
    std::string s;
  };

}

#endif // _CROW_HPP_

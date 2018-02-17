#ifndef _SIMPLEPB_HPP_
#define _SIMPLEPB_HPP_

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

namespace simplepb {

  class Encoder {
  public:
    virtual void put(uint32_t fieldIndex, int32_t value) = 0;
    virtual void put(uint32_t fieldIndex, uint32_t value) = 0;
    virtual void put(uint32_t fieldIndex, int64_t value) = 0;
    virtual void put(uint32_t fieldIndex, uint64_t value) = 0;
    virtual void put(uint32_t fieldIndex, double value) = 0;
    virtual void put(uint32_t fieldIndex, const std::string value) = 0;
    virtual void put(uint32_t fieldIndex, const char* str) = 0;
    virtual void put(uint32_t fieldIndex, const std::vector<uint8_t>& value) = 0;
    virtual void put(uint32_t fieldIndex, bool value) = 0;
    virtual void putRowSep() = 0;

    virtual std::string str()=0;
    virtual void clear() = 0;

    virtual ~Encoder() {}
  };

  class DecoderListener {
  public:
    virtual void onField(uint32_t fieldIndex, int32_t value) {}
    virtual void onField(uint32_t fieldIndex, uint32_t value) {}
    virtual void onField(uint32_t fieldIndex, int64_t value) {}
    virtual void onField(uint32_t fieldIndex, uint64_t value) {}
    virtual void onField(uint32_t fieldIndex, double value) {}
    virtual void onField(uint32_t fieldIndex, const std::string &value) {}
    virtual void onField(uint32_t fieldIndex, const std::vector<uint8_t> value) {}
    virtual void onRowSep() {}
  };

  class Decoder {
  public:
    virtual bool decode(const Bytes &encodedSrc, DecoderListener &listener) = 0;
    virtual ~Decoder() {}
  };

  Encoder* EncoderNew();
  Decoder* DecoderNew();


  class DecoderLogger : public DecoderListener {
  public:
    DecoderLogger() : s() {}
    virtual void onField(uint32_t fieldIndex, int32_t value) {
      char tmp[128];
      sprintf(tmp,"[%2d] int32 %d,", fieldIndex, value);
      s += std::string(tmp);
     }
    virtual void onField(uint32_t fieldIndex, uint32_t value) {
      char tmp[128];
      sprintf(tmp,"[%2d] uint32 %u,", fieldIndex, value);
      s += std::string(tmp);
    }
    virtual void onField(uint32_t fieldIndex, int64_t value) {
      char tmp[128];
      sprintf(tmp,"[%2d] int64 %lld,", fieldIndex, value);
      s += std::string(tmp);
    }
    virtual void onField(uint32_t fieldIndex, uint64_t value) {
      char tmp[128];
      sprintf(tmp,"[%2d] uint64 %llu,", fieldIndex, value);
      s += std::string(tmp);
    }
    virtual void onField(uint32_t fieldIndex, double value) {
      char tmp[128];
      sprintf(tmp,"[%2d] double %.3f,", fieldIndex, value);
      s += std::string(tmp);
    }
    virtual void onField(uint32_t fieldIndex, const std::string &value) {
      char tmp[128];
      sprintf(tmp,"[%2d] \"", fieldIndex);
      s += std::string(tmp);
      s += value;
      s += "\",";
    }
    virtual void onField(uint32_t fieldIndex, const std::vector<uint8_t> value) {
      char tmp[128];
      sprintf(tmp,"[%2d] bytes [%lu],", fieldIndex, value.size());
      s += std::string(tmp);
    }

    std::string str() { return s; }
  private:
    std::string s;
  };

}

#endif // _SIMPLEPB_HPP_

#ifndef _CROW_HPP_
#define _CROW_HPP_

#include <stdint.h>
#include <string>
#include <vector>

enum CrowTag {
    TUNKNOWN,
    TBLOCK,          // 1 optional
    TTABLE,          // 2 optional

    THFIELD,         // 3
    THREF,           // 4

    TROW,            // 5
    TREF,            // 6
    TFLAGS,          // 7

    NUMTAGS
};

/*
  the tagid byte has following scenarios:

  1nnn nnnn    Upper bit set - lower 7 bits contain index of field, value bytes follow
  000F 0001    TFIELDINFO, if bit 4 set, no value follows definition
  0FFF 0110    TFLAGS, bits 4-6 contain app specific flags
  0FFF 0011    TROWSEP, bits 4-6 contain app specific flags
  0FFF 0101    TSETREF, bits 4-6 contain app specific flags
  0000 TTTT    Tagid in bits 0-3
*/

#define FIELDINFO_FLAG_RAW  (uint8_t)0x10
#define FIELDINFO_FLAG_HAS_SUBID (uint8_t)0x20
#define FIELDINFO_FLAG_HAS_NAME  (uint8_t)0x40

enum CrowType {
    NONE,
    TSTRING,         // 1
    TINT32,          // 2
    TUINT32,         // 3
    TINT64,          // 4
    TUINT64,         // 5
    TINT16,          // 6
    TUINT16,         // 7
    TINT8,           // 8
    TUINT8,          // 9

    TFLOAT32,        // 10
    TFLOAT64,        // 11
    TBYTES,          // 12

    NUM_TYPES
};

typedef std::vector<uint8_t> Bytes;

namespace crow {
  class Field {
  public:
    CrowType    typeId;
    uint32_t    id;
    uint32_t    subid;
    std::string name;
    uint8_t     index;

    bool        isRaw;     // struct field
    uint32_t    fixedLen;

    bool        _written;
  };

  inline CrowType type_for(double val) { return TFLOAT64; }
  inline CrowType type_for(float val) { return TFLOAT32; }
  inline CrowType type_for(const std::string val) { return TSTRING; }
  inline CrowType type_for(uint8_t val) { return TUINT8; }
  inline CrowType type_for(int8_t val) { return TINT8; }
  inline CrowType type_for(uint16_t val) { return TUINT16; }
  inline CrowType type_for(int16_t val) { return TINT16; }
  inline CrowType type_for(uint32_t val) { return TUINT32; }
  inline CrowType type_for(int32_t val) { return TINT32; }
  inline CrowType type_for(uint64_t val) { return TUINT64; }
  inline CrowType type_for(int64_t val) { return TINT64; }
  inline CrowType type_for(bool val) { return TUINT8; }
  inline CrowType type_for(const char * val) { return TSTRING; }
  inline CrowType type_for(const uint8_t * val) { return TBYTES; }
  inline CrowType type_for(const std::vector<uint8_t>& value) { return TBYTES; }

  inline size_t byte_size(CrowType typeId)
  {
    switch(typeId) {
      case TINT8: return 1;
      case TUINT8: return 1;
      case TINT16: return 2;
      case TUINT16: return 2;
      case TINT32: return 4;
      case TUINT32: return 4;
      case TINT64: return 8;
      case TUINT64: return 8;
      case TFLOAT32: return 4;
      case TFLOAT64: return 8;
      default:
      return 0;
    }
  }

}

#include "crow/crow_encode.hpp"
#include "crow/crow_decode.hpp"

#include "crow/private/crow_encode_impl.hpp"
#include "crow/private/crow_decode_impl.hpp"

#endif // _CROW_HPP_

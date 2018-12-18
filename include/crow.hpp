#ifndef _CROW_HPP_
#define _CROW_HPP_

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

#include "../dyno/include/dynobj.hpp"

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
  0FFF 0001    TFIELDINFO, FIELDINFO_FLAG_XX apply
  0FFF 0110    TFLAGS, bits 4-6 contain app specific flags
  0FFF 0011    TROWSEP, bits 4-6 contain app specific flags
  0FFF 0101    TSETREF, bits 4-6 contain app specific flags
  0FFF 0010    TTABLE, TABLE_FLAG_XX apply
  0000 TTTT    Tagid in bits 0-3
*/

#define FIELDINFO_FLAG_RAW  (uint8_t)0x10
#define FIELDINFO_FLAG_HAS_SUBID (uint8_t)0x20
#define FIELDINFO_FLAG_HAS_NAME  (uint8_t)0x40

// Table flags
// DECORATE: this table defines context / decorator fields that apply to tables in this block

#define TABLE_FLAG_DECORATE  (uint8_t)0x10

typedef DynType CrowType;

typedef std::vector<uint8_t> Bytes;

namespace crow {

  /*
   * Encoder and Decoders need to know a few more things about the field.
   */
  struct FieldInfo : public FieldDef {
  public:

    uint32_t    structFieldLength; // if > 0, part of struct

    uint8_t     index;             // Nth field defined, used in value encoding refs
    bool        isWritten;         // If field def has been written to encoded output yet

    bool isStructField() const { return structFieldLength > 0; }

    FieldInfo(const SPFieldDef def, uint8_t idx, uint32_t fixedLen) :
      FieldDef(def->typeId, def->name, def->id, def->schemaId),
      structFieldLength(fixedLen), index(idx), isWritten(false) {
    }
  };

  typedef std::shared_ptr<FieldInfo> SPFieldInfo;
  typedef std::shared_ptr<const FieldInfo> SPCFieldInfo;

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

} // namespace crow

#include "crow/crow_encode.hpp"
#include "crow/crow_decode.hpp"

#include "crow/private/crow_encode_impl.hpp"
#include "crow/private/crow_decode_impl.hpp"

#endif // _CROW_HPP_

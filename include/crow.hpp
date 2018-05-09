#ifndef _CROW_HPP_
#define _CROW_HPP_

#include <stdint.h>
#include <string>
#include <vector>

enum CrowTag {
    TUNKNOWN,
    TFIELDINFO,      // 1
    TBLOCK,          // 2
    TROWSEP,         // 3
    TSET,            // 4
    TSETREF,         // 5
    TFLAGS,          // 6

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

#define FIELDINFO_FLAG_NO_VALUE  (uint8_t)0x10
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

  class Encoder {
  public:
    virtual Field *fieldFor(CrowType type, uint32_t id, uint32_t subid = 0) = 0;
    virtual Field *fieldFor(CrowType type, std::string name) = 0;
    inline Field *fieldFor(CrowType type, uint32_t id, uint32_t subid, std::string name) {
      return (id > 0 ? fieldFor(type, id, subid) : fieldFor(type, name));
    }

    virtual void put(const Field *pField, int32_t value) = 0;
    virtual void put(const Field *pField, uint32_t value) = 0;
    virtual void put(const Field *pField, int64_t value) = 0;
    virtual void put(const Field *pField, uint64_t value) = 0;
    virtual void put(const Field *pField, int8_t value) = 0;
    virtual void put(const Field *pField, uint8_t value) = 0;
    virtual void put(const Field *pField, int16_t value) = 0;
    virtual void put(const Field *pField, uint16_t value) = 0;
    virtual void put(const Field *pField, double value) = 0;
    virtual void put(const Field *pField, float value) = 0;
    virtual void put(const Field *pField, const std::string value) = 0;
    virtual void put(const Field *pField, const char* str) = 0;
    virtual void put(const Field *pField, const std::vector<uint8_t>& value) = 0;
    virtual void put(const Field *pField, const uint8_t* bytes, size_t len) = 0;
    virtual void put(const Field *pField, bool value) = 0;

    struct EncField {
    public:
      EncField(Encoder* _pEnc, uint32_t _id, uint32_t _subid = 0) : id(_id), subid(_subid), name(), pEnc(_pEnc) {}
      EncField(Encoder* _pEnc, std::string _name) : id(0), subid(0), name(_name), pEnc(_pEnc) {}

      void operator=(bool value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), (uint8_t)(value ? 1 : 0));
      }

      void operator=(uint32_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(int32_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(uint64_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(int64_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(uint16_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(int16_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(uint8_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(int8_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(double value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(float value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(const std::string value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(const char* value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(const std::vector<uint8_t>& value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }

      EncField& operator[](int _subid) {
        subid = _subid;
        return *this;
      }

      EncField& operator[](uint32_t _subid) {
        subid = _subid;
        return *this;
      }

      uint32_t id;
      uint32_t subid;
      std::string name;
      Encoder* pEnc;
    };

    EncField operator[] (uint32_t id) {
      return EncField(this, id);
    }
    EncField operator[] (int id) {
      return EncField(this, (uint32_t)id);
    }
    EncField operator[] (std::string name) {
      return EncField(this, name);
    }
    EncField operator[] (uint64_t id_and_subid) {
      uint32_t id = id_and_subid & 0x0FFFFFFFFL;
      uint32_t subid = (id_and_subid >> 32) & 0x0FFFFFFFFL;
      return EncField(this, id, subid);
    }

    inline void put(int32_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(uint32_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(int64_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(uint64_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(int8_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(uint8_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(int16_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(uint16_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(double value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(float value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(const std::string value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(const char* str, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(str), id, subid), str);
    }
    inline void put(const std::vector<uint8_t>& value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(TBYTES, id, subid), value);
    }
    inline void put(const uint8_t* bytes, size_t len, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(TBYTES, id, subid), bytes, len);
    }
    inline void put(bool value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }

    inline void put(int32_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(uint32_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(int64_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(uint64_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(int8_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(uint8_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(int16_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(uint16_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(double value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(float value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(const std::string value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(const char* str, const std::string name) {
      put(fieldFor(crow::type_for(str), name), str);
    }
    inline void put(const std::vector<uint8_t>& value, const std::string name) {
      put(fieldFor(TBYTES, name), value);
    }
    inline void put(const uint8_t* bytes, size_t len, const std::string name) {
      put(fieldFor(TBYTES, name), bytes, len);
    }
    inline void put(bool value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }

    virtual void putRowSep(uint8_t flags = 0) = 0;

    inline void endRow(uint8_t flags = 0) { putRowSep(flags); }

    virtual void setFlags(uint8_t flags) = 0;

    /**
     * @brief Places a set of fields that can be referenced rather than placed in-line
     * for future occurances.
     *
     * A 'setref' is a set of field values that can be referenced,
     * rather than being placed in-line.  Wide tables in a non-relational
     * databases result in repetitive data.  By using setref during the encoding
     * process, the stored data is more compact, while being transparent to the
     * application receiving the decoded data.
     *
     * NOTE: A set cannot include any setrefs.
     *
     * For example, a table of People may contains fields for
     * Company (company_name, company_category, etc) and
     * Location (country_code, city_name, geo_lat, geo_long, zipcode, etc).  By
     * using setrefs, the encoding saves space if the same company and location
     * data occurs more than once.
     *
     * @param fieldIndexAdd fieldIndexAdd will be added to the value of fieldIndex of each field in set.
     * @param setId An ID used by encoding application for get/put.
     *
     * @return true on error, false on success.
     */
    virtual void    startSet() = 0;
    virtual uint8_t endSet() = 0;

    /**
     * @brief If found (previous call to putSet(setId) was made), places SETREF.
     *
     * @returns true on error, false on success.
     */
    virtual void    putSetRef(uint8_t setId, uint8_t flags = 0) = 0;

    virtual const uint8_t* data() const = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;

    virtual ~Encoder() {}
  };

  class DecoderListener {
  public:
    virtual void onField(const Field *pField, int32_t value, uint8_t flags) {}
    virtual void onField(const Field *pField, uint32_t value, uint8_t flags) {}
    virtual void onField(const Field *pField, int64_t value, uint8_t flags) {}
    virtual void onField(const Field *pField, uint64_t value, uint8_t flags) {}
    virtual void onField(const Field *pField, double value, uint8_t flags) {}
    virtual void onField(const Field *pField, const std::string &value, uint8_t flags) {}
    virtual void onField(const Field *pField, const std::vector<uint8_t> value, uint8_t flags) {}
    virtual void onRowSep() {}
  };

  class Decoder {
  public:

    /**
     * @brief Decode a single row
     * @return false on success, true if no data remaining or error.
     */
    virtual bool decodeRow(DecoderListener &listener) = 0;

    /**
     * @brief Decode all data and return number of rows.
     */
    virtual uint32_t decode(DecoderListener &listener, uint64_t setId = 0) = 0;

    //virtual void init(const uint8_t* pEncData, size_t encLength) = 0;

    virtual ~Decoder() {}

    /**
     * returns 0 if no error, otherwise code from errno.h
     */
    virtual int getErrCode() const = 0;

    /**
     * returns bitwise OR of all TYPE_XX encountered in decoding.
     */
    virtual uint64_t getTypeMask() const = 0;

    /**
     * Indicate that value will be added to the fieldIndex of every decoded field.
     * Used for sets of bi-directional data.
     */
    //virtual void setFieldIndexAdd(uint32_t value) = 0;

    /**
     * After decoding is finished, returns number of bytes after placing sets
     */
    virtual size_t getExpandedSize() = 0;

    virtual std::vector<Field> getFields() = 0;
  };

  Encoder* EncoderNew(size_t initialCapacity = 4096);
  Decoder* DecoderNew(const uint8_t* pEncData, size_t encLength);

  /* struct to encapsulate any decoded column value.  Used by GenericDecoderListener */

  struct DecColValue {
    uint8_t fieldIndex;
    CrowType typeId;
    uint8_t flags;
    union {
      int32_t i32val;
      uint32_t u32val;
      int64_t i64val;
      uint64_t u64val;
      int8_t i8val;
      uint8_t u8val;
      double dval;
    };
    std::string strval;

    DecColValue() : fieldIndex(0), typeId(CrowType::NONE), flags(0), i64val(0), strval() {}

    DecColValue(uint8_t idx, CrowType typ, std::string sval, uint8_t flgs) : fieldIndex(idx), typeId(typ),
      flags(flgs), u64val(0), strval(sval) {
    }
  };

  typedef std::map<uint8_t, DecColValue> GenDecRow;

  class GenericDecoderListener : public DecoderListener {
  public:

    GenericDecoderListener() : _rows(), _rownum(0) {
    }
    void onField(const Field *pField, int32_t value, uint8_t flags) override {
      sprintf(tmp,"%d", value);
      auto v = DecColValue(pField->index, pField->typeId, std::string(tmp), flags);
      v.i32val = value;
      _addValue(v);
    }
    void onField(const Field *pField, uint32_t value, uint8_t flags) override {
      sprintf(tmp,"%u", value);
      auto v = DecColValue(pField->index, pField->typeId, std::string(tmp), flags);
      v.u32val = value;
      _addValue(v);
    }
    void onField(const Field *pField, int64_t value, uint8_t flags) override {
      sprintf(tmp,"%lld", value);
      auto v = DecColValue(pField->index, pField->typeId, std::string(tmp), flags);
      v.i64val = value;
      _addValue(v);
    }
    void onField(const Field *pField, uint64_t value, uint8_t flags) override {
      sprintf(tmp,"%llu", value);
      auto v = DecColValue(pField->index, pField->typeId, std::string(tmp), flags);
      v.u64val = value;
      _addValue(v);
    }
    void onField(const Field *pField, double value, uint8_t flags) override {
      sprintf(tmp,"%.3f", value);
      auto v = DecColValue(pField->index, pField->typeId, std::string(tmp), flags);
      v.dval = value;
      _addValue(v);
    }
    void onField(const Field *pField, const std::string &value, uint8_t flags) override {
      auto v = DecColValue(pField->index, pField->typeId, value, flags);
      _addValue(v);
    }
    void onField(const Field *pField, const std::vector<uint8_t> value, uint8_t flags) override {
      auto v = DecColValue(pField->index, pField->typeId, "", flags);
      v.strval.assign((const char *)value.data(), value.size());
      _addValue(v);
    }

    void onRowSep() override { _rownum++; }

    std::vector< GenDecRow > _rows;

  private:

    void _addValue(DecColValue val) {
      if (_rows.size() <= _rownum) {
        _rows.push_back(GenDecRow());
      }
      _rows[_rows.size()-1][val.fieldIndex] = val;
    }

    size_t _rownum;
    char tmp[64];
  };


}

#endif // _CROW_HPP_

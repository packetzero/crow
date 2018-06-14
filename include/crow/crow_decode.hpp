#ifndef _CROW_DECODE_HPP_
#define _CROW_DECODE_HPP_

#include <stdint.h>
#include <string>
#include <vector>


namespace crow {

  const int RV_SKIP_VARIABLE_FIELDS = 2;

  class DecoderListener {
  public:
    virtual void onField(const Field *pField, int32_t value, uint8_t flags) {}
    virtual void onField(const Field *pField, uint32_t value, uint8_t flags) {}
    virtual void onField(const Field *pField, int64_t value, uint8_t flags) {}
    virtual void onField(const Field *pField, uint64_t value, uint8_t flags) {}
    virtual void onField(const Field *pField, double value, uint8_t flags) {}
    virtual void onField(const Field *pField, const std::string &value, uint8_t flags) {}
    virtual void onField(const Field *pField, const std::vector<uint8_t> value, uint8_t flags) {}
    virtual void onRowStart() {}
    virtual void onRowEnd() {}
    /**
     * @returns 0 by default.  If RV_SKIP_VARIABLE_FIELDS returned,
     * decoder will skip over variable length fields associated with row.
     */
    virtual int onStruct(const uint8_t *data, size_t datalen) { return 0;}
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

    GenericDecoderListener() : _rows(), _structData(), _rownum(0) {
    }
    int onStruct(const uint8_t* data, size_t datalen) override {
      auto index = _structData.size();
      _structData.push_back(std::vector<uint8_t>(datalen));
      memcpy(_structData[index].data(), data, datalen);
      return 0;
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

    void onRowEnd() override { _rownum++; }

    std::vector< GenDecRow > _rows;
    std::vector< std::vector<uint8_t> > _structData;

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

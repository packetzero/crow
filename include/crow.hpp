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
  TYPE_SET            = 10,
  TYPE_SETREF         = 11,
  MAX_FIELD_TYPE      = 12,
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
    virtual bool putSet(uint64_t setId, Encoder& encForSet, uint32_t fieldIndexAdd = 0U) = 0;

    /**
     * @brief If found (previous call to putSet(setId) was made), places SETREF.
     *
     * @returns true on error, false on success.
     */
    virtual bool putSetRef(uint64_t setId, uint32_t fieldIndexAdd = 0U) = 0;

    virtual const uint8_t* data() const = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;

    virtual ~Encoder() {}
  };

  class DecoderListener {
  public:
    virtual void onField(uint32_t fieldIndex, int32_t value, const std::string name_optional="", uint64_t setId=0) {}
    virtual void onField(uint32_t fieldIndex, uint32_t value, const std::string name_optional="", uint64_t setId=0) {}
    virtual void onField(uint32_t fieldIndex, int64_t value, const std::string name_optional="", uint64_t setId=0) {}
    virtual void onField(uint32_t fieldIndex, uint64_t value, const std::string name_optional="", uint64_t setId=0) {}
    virtual void onField(uint32_t fieldIndex, double value, const std::string name_optional="", uint64_t setId=0) {}
    virtual void onField(uint32_t fieldIndex, const std::string &value, const std::string name_optional="", uint64_t setId=0) {}
    virtual void onField(uint32_t fieldIndex, const std::vector<uint8_t> value, const std::string name_optional="", uint64_t setId=0) {}
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

    /**
     * @brief Scans for next RowSep tag, returns true if found.
     *   Keeps track of bytes for entire row, for use by subsequent
     *   calls to decodeField().
     */
    virtual bool scanRow() = 0;

    /**
     * @brief After a call to scanRow()==false, decodeField() can be used
     *  to decode the first entry matching fieldIndex.
     * @returns true if found, false otherwise.
     */
    virtual bool decodeField(uint32_t fieldIndex, DecoderListener& listener) = 0;

    /**
     * @brief Get highest fieldIndex in row.
     */
    virtual uint32_t getLastFieldIndex() = 0;

    virtual void init(const uint8_t* pEncData, size_t encLength) = 0;

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
    virtual void setFieldIndexAdd(uint32_t value) = 0;

    /**
     * After decoding is finished, returns number of bytes after placing sets
     */
    virtual size_t getExpandedSize() = 0;
  };

  Encoder* EncoderNew(size_t initialCapacity = 4096);
  Decoder* DecoderNew(const uint8_t* pEncData, size_t encLength);
/*
  class DecoderLogger : public DecoderListener {
  public:
    DecoderLogger() : s() {}
    void onField(uint32_t fieldIndex, int32_t value, const std::string name_optional, uint64_t setId) override {
      sprintf(tmp,"int32 %d", value);
      _render(fieldIndex, std::string(tmp), name_optional, setId);
     }
    void onField(uint32_t fieldIndex, uint32_t value, const std::string name_optional, uint64_t setId) override {
      sprintf(tmp,"uint32 %u", value);
      _render(fieldIndex, std::string(tmp), name_optional, setId);
    }
    void onField(uint32_t fieldIndex, int64_t value, const std::string name_optional, uint64_t setId) override {
      sprintf(tmp,"int64 %lld", value);
      _render(fieldIndex, std::string(tmp), name_optional, setId);
    }
    void onField(uint32_t fieldIndex, uint64_t value, const std::string name_optional, uint64_t setId) override {
      sprintf(tmp,"uint64 %llu", value);
      _render(fieldIndex, std::string(tmp), name_optional, setId);
    }
    void onField(uint32_t fieldIndex, double value, const std::string name_optional, uint64_t setId) override {
      sprintf(tmp,"double %.3f", value);
      _render(fieldIndex, std::string(tmp), name_optional, setId);
    }
    void onField(uint32_t fieldIndex, const std::string &value, const std::string name_optional, uint64_t setId) override {
      sprintf(tmp, "\"%s\"", value.c_str());
      _render(fieldIndex, std::string(tmp), name_optional, setId);
    }
    void onField(uint32_t fieldIndex, const std::vector<uint8_t> value, const std::string name_optional, uint64_t setId) override {
      sprintf(tmp,"bytes [%lu]", value.size());
      _render(fieldIndex, std::string(tmp), name_optional, setId);
    }

    std::string str() { return s; }
  private:

    void _render(uint32_t fieldIndex, const std::string val, const std::string name_optional, uint64_t setId) {
      sprintf(tmp, "[%2d]%s %s", fieldIndex, (name_optional.length() > 0 ? name_optional.c_str() : ""), val.c_str());
      s += std::string(tmp);
      if (setId > 0) { sprintf(tmp, " SET:%llx", setId); s += std::string(tmp);}
      s += ",";
    }

    char tmp[256];
    std::string s;
  };
*/
  /* struct to encapsulate any decoded column value.  Used by GenericDecoderListener */

  struct DecColValue {
    uint32_t fieldIndex;
    FieldType fieldType;
    std::string name;
    uint64_t setId;
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

    DecColValue(uint32_t fidx, FieldType ftype, std::string namo, uint64_t sid,
      std::string sval) : fieldIndex(fidx), fieldType(ftype), name(namo),
      setId(sid), u64val(0), strval(sval) {
    }
  };

  class GenericDecoderListener : public DecoderListener {
  public:
    GenericDecoderListener() : _rows() {
    }
    void onField(uint32_t fieldIndex, int32_t value, const std::string name_optional, uint64_t setId) override {
      sprintf(tmp,"%d", value);
      auto v = DecColValue(fieldIndex, TYPE_INT32, name_optional, setId, std::string(tmp));
      v.i32val = value;
      _addValue(v);
    }
    void onField(uint32_t fieldIndex, uint32_t value, const std::string name_optional, uint64_t setId) override {
      sprintf(tmp,"%u", value);
      auto v = DecColValue(fieldIndex, TYPE_UINT32, name_optional, setId, std::string(tmp));
      v.u32val = value;
      _addValue(v);
    }
    void onField(uint32_t fieldIndex, int64_t value, const std::string name_optional, uint64_t setId) override {
      sprintf(tmp,"%lld", value);
      auto v = DecColValue(fieldIndex, TYPE_INT64, name_optional, setId, std::string(tmp));
      v.i64val = value;
      _addValue(v);
    }
    void onField(uint32_t fieldIndex, uint64_t value, const std::string name_optional, uint64_t setId) override {
      sprintf(tmp,"%llu", value);
      auto v = DecColValue(fieldIndex, TYPE_UINT64, name_optional, setId, std::string(tmp));
      v.u64val = value;
      _addValue(v);
    }
    void onField(uint32_t fieldIndex, double value, const std::string name_optional, uint64_t setId) override {
      sprintf(tmp,"%.3f", value);
      auto v = DecColValue(fieldIndex, TYPE_DOUBLE, name_optional, setId, std::string(tmp));
      v.dval = value;
      _addValue(v);
    }
    void onField(uint32_t fieldIndex, const std::string &value, const std::string name_optional, uint64_t setId) override {
      auto v = DecColValue(fieldIndex, TYPE_STRING, name_optional, setId, value);
      _addValue(v);
    }
    void onField(uint32_t fieldIndex, const std::vector<uint8_t> value, const std::string name_optional, uint64_t setId) override {
      auto v = DecColValue(fieldIndex, TYPE_INT32, name_optional, setId, "");
      v.strval.assign((const char *)value.data(), value.size());
      _addValue(v);
    }

    void onRowSep() override { _rows.push_back(std::vector<DecColValue>()); }

    std::string str() {
      std::string s;
      for (auto &row : _rows) {
        for (auto &col : row) {
          s += _render(col.fieldIndex, col.strval, col.name, col.setId, col.fieldType);
        }
      }
      return s;
    }
    
    std::string _typeName(FieldType ft) {
      switch(ft) {
        case TYPE_BOOL: return "bool";
        case TYPE_INT32: return "int32";
        case TYPE_UINT32: return "uint32";
        case TYPE_INT64: return "int64";
        case TYPE_UINT64: return "uint64";
        case TYPE_DOUBLE: return "double";
        default:
          break;
      }
      return "";
    }

    std::string _render(uint32_t fieldIndex, const std::string val, const std::string name_optional, uint64_t setId, FieldType ft) {
      char tmp[128];
      sprintf(tmp, "[%2d]%s %s %s%s%s", fieldIndex, (name_optional.length() > 0 ? name_optional.c_str() : ""), _typeName(ft).c_str(), (ft == TYPE_STRING ? "\"" : ""), val.c_str(), (ft == TYPE_STRING ? "\"" : ""));
      std::string s = std::string(tmp);
      if (setId > 0) { sprintf(tmp, " SET:%llx", setId); s += std::string(tmp);}
      s += ",";
      return s;
    }
    std::vector< std::vector<DecColValue > > _rows;

  private:

    void _addValue(DecColValue& val) {
      if (_rows.size() == 0) { _rows.push_back(std::vector<DecColValue>()); }
      _rows[_rows.size()-1].push_back(val);
    }

    char tmp[64];
  };


}

#endif // _CROW_HPP_

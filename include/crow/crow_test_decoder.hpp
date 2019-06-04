#ifndef _CROW_TEST_DECODER_HPP_
#define _CROW_TEST_DECODER_HPP_

#include "crow_decode.hpp"

namespace crow {
  /* struct to encapsulate any decoded column value.  Used by GenericDecoderListener */

  struct DecColValue : public DynVal {
//    uint8_t fieldIndex;
    uint8_t flags;

    DecColValue() : DynVal(), flags(0) {}

    DecColValue(int32_t val) : DynVal(val) {}
    DecColValue(uint32_t val) : DynVal(val) {}
    DecColValue(int64_t val) : DynVal(val) {}
    DecColValue(uint64_t val) : DynVal(val) {}
    DecColValue(int16_t val) : DynVal(val) {}
    DecColValue(uint16_t val) : DynVal(val) {}
    DecColValue(int8_t val) : DynVal(val) {}
    DecColValue(uint8_t val) : DynVal(val) {}
    DecColValue(double val) : DynVal(val) {}
    DecColValue(float val) : DynVal(val) {}
    DecColValue(const std::string val) : DynVal(val) {}
    DecColValue(const char *val) : DynVal(val) {}
    DecColValue(const std::vector<uint8_t> val) : DynVal(val) {}


  };

  typedef std::map<SPCFieldInfo, DecColValue> GenDecRow;

  class GenericDecoderListener : public DecoderListener {
  public:

    GenericDecoderListener() : _rows(), _structData(), _decoratorFields(), _rownum(0), _structFields(), _tableFlags(0) {
    }
    int onStruct(const uint8_t* data, size_t datalen, const std::vector<SPCFieldInfo> &structFields) override {
      auto index = _structData.size();
      _structData.push_back(std::vector<uint8_t>(datalen));
      memcpy(_structData[index].data(), data, datalen);

      _structFields.clear();
      for (auto fi : structFields) { _structFields.push_back(fi); }
//      _structFields = structFields;
      return 0;
    }
    virtual void onField(SPCFieldInfo fieldDef, int8_t value, uint8_t flags) override {
      _addValue(fieldDef, DecColValue(value), flags);
    }
    virtual void onField(SPCFieldInfo fieldDef, uint8_t value, uint8_t flags) override {
      _addValue(fieldDef, DecColValue(value), flags);
    }

    void onField(SPCFieldInfo fieldDef, int32_t value, uint8_t flags) override {
      _addValue(fieldDef, DecColValue(value), flags);
    }
    void onField(SPCFieldInfo fieldDef, uint32_t value, uint8_t flags) override {
      _addValue(fieldDef, DecColValue(value), flags);
    }
    void onField(SPCFieldInfo fieldDef, int64_t value, uint8_t flags) override {
      _addValue(fieldDef, DecColValue(value), flags);
    }
    void onField(SPCFieldInfo fieldDef, uint64_t value, uint8_t flags) override {
      _addValue(fieldDef, DecColValue(value), flags);
    }
    void onField(SPCFieldInfo fieldDef, double value, uint8_t flags) override {
      _addValue(fieldDef, DecColValue(value), flags);
    }
    void onField(SPCFieldInfo fieldDef, const std::string &value, uint8_t flags) override {
      _addValue(fieldDef, DecColValue(value), flags);
    }
    void onField(SPCFieldInfo fieldDef, const std::vector<uint8_t> value, uint8_t flags) override {
      auto v = DecColValue(value);
      _addValue(fieldDef,v,flags);
    }

    void onTableStart(uint8_t flags) override {
      _tableFlags = flags;
    }

    void onRowEnd(bool isHeaderRow, const uint8_t* pEncodedRowStart, size_t length) override {
      if (!isHeaderRow) {
        _rownum++;
      }
    }

    std::vector< GenDecRow > _rows;
    std::vector< std::vector<uint8_t> > _structData;
    GenDecRow _decoratorFields;

  private:

    void _addValue(SPCFieldInfo fieldDef, DecColValue val, uint8_t flags) {
      val.flags = flags;
      if (_tableFlags & TABLE_FLAG_DECORATE) {
        // add decorator value
        _decoratorFields[fieldDef] = val;
      } else {
        // add row if needed
        if (_rows.size() <= _rownum) {
          _rows.push_back(GenDecRow());
        }
        // add column value to row
        GenDecRow &row = _rows[_rows.size()-1];
        row[fieldDef] = val;
      }
    }

    size_t _rownum;
    std::vector<SPCFieldInfo> _structFields;
    uint8_t _tableFlags;
  };

}

#endif // _CROW_TEST_DECODER_HPP_

#ifndef _CROW_DECODE_IMPL_HPP_
#define _CROW_DECODE_IMPL_HPP_

#include <map>
#include <errno.h>

#include "../../crow.hpp"
#include "stack.hpp"
#include "protobuf_wire_format.h"
#include "../../crow/crow_test_decoder.hpp"

#define NONE_LEFT(PTR) (PTR >= _end)
#define BYTES_REMAIN(PTR) (PTR < _end)
#define MAX_NAME_LEN 64
#define MAX_FIELDS   72    // ridiculously wide table

#define MAX_SANE_SET_SIZE 32000000 // 32 MB should be plenty

#define DECORATOR_INDEX_OFFSET 100

#define NOT_SKIP_MODE (0 == (_modeFlags & DECODER_MODE_SKIP))

namespace crow {

  /*
   * Used to capture SET context for use when SETREF are encountered during decoding.
   */
  struct SetContext {
    uint64_t             _setId;
    std::vector<uint8_t> _vec;

    SetContext(uint64_t setId, const uint8_t* src, size_t len) : _setId(setId), _vec(len) {
      memcpy(_vec.data(), src, len);
    }

    const uint8_t* data() const { return _vec.data(); }
    size_t size() const { return _vec.size(); }
  };


  struct PData
  {
    const uint8_t*  start;
    const uint8_t*  ptr;
    const uint8_t*  end;
    size_t          length;

    size_t remaining() { return (size_t)(end - ptr); }
    bool empty() { return (ptr >= end); }
    size_t getOffset() { return (size_t)(ptr - start); }

    PData(const uint8_t* p, size_t len) : start(p), ptr(p), end(p + len), length(len) { }

  };

  /*
   * Implementation of Decoder
   */
  class DecoderImpl : public Decoder {
  public:
    /*
     * DecoderImpl constructor
     */
    DecoderImpl(const uint8_t* pEncData, size_t encLength) : Decoder(), _data(pEncData, encLength), _fields(), _err(0), _typemask(0L),
      _errOffset(0L), _setId(0L), _mapSets(),
      _byteCount(encLength), _flags(0), _numRows(0),
      _structFields(), _structLen(0), _rowStartPos(0), _modeFlags(0),
      _tableFlags(0)
      //, _isDecoratorTable(false),
    //_decoratorFields(), _decoratorListener(), _decoratorValues()
    {
    }

    ~DecoderImpl() {
      auto it = _mapSets.begin();
      while (it != _mapSets.end()) {
        delete it->second;
        _mapSets.erase(it++);
      }
    }
    void setModeFlags(int flags) override { _modeFlags = flags; }

    /*
     * decode
     */
    uint32_t decode(DecoderListener &listener, uint64_t setId = 0L) override {
      _setId = setId;

      _numRows = 0;
      _rowStartPos = _data.getOffset();
      while(false == decodeRow(listener)) {
          _numRows++;
      }

      if (_numRows > 0) {
        listener.onRowEnd(false, _data.start + _rowStartPos, (size_t)(_data.getOffset() - _rowStartPos));
      }

      return _numRows;
    }

    bool decodeRow(DecoderListener &listener) override {
      if (_modeFlags & DECODER_MODE_SKIP) {
        return _doSkipRow(listener, _data);
      } else {
        return _doDecodeRow(listener, _data);
      }
    }

    bool _doDecodeRow(DecoderListener &listener, PData &data) {
      while (true) {

        uint8_t tagbyte;
        if (data.empty()) { _markError(ENOSPC, data); return true; }
        tagbyte = *data.ptr++;
        bool isIndex = (tagbyte & (uint8_t)0x80) != 0;
        uint8_t tagid = tagbyte & 0x0F;

        if (isIndex) {

          uint8_t index = tagbyte & (uint8_t)0x7F;
          if (index >= _fields.size()) {
            _markError(EINVAL, data); return true;
          }
          SPFieldInfo pField = _fields[index];

          if (_decodeValue(pField, data, listener)) {
            break;
          }

        } else if (tagid == TROW) {
          listener.onRowEnd((_numRows == 0), _data.start + _rowStartPos, (size_t)(_data.getOffset() - _rowStartPos - 1));

          _flags = (tagbyte >> 4) & 0x07;
          _rowStartPos = data.getOffset();
          { listener.onRowStart(); }
          if (_structLen > 0) {
            auto structPtr = data.ptr;
            if (data.remaining() < _structLen) {
              throw new std::runtime_error("no more data, trying to read struct");
            }
            data.ptr += _structLen;
            // read struct
            size_t varlen = 0;
            if (_fields.size() > _structFields.size()) {
              // read length of variable length section
              varlen = readVarInt(data);
              if (varlen > data.remaining()) {
                throw new std::runtime_error("length of variable fields extends past end");
              }
            }

            int rv = listener.onStruct(structPtr, _structLen, _constStructFields);
            if (rv == RV_SKIP_VARIABLE_FIELDS) {
              data.ptr += varlen;
            }
          }

          break;

        } else if (tagid == TFLAGS) {

          _flags = (tagbyte >> 4) & 0x07;

        } else if (tagid == TBLOCK) {

          // TODO

        } else if (tagid == TTABLE) {


          // TODO: snapshot decorators
          // clear previous table state.
          _structFields.clear();
          _fields.clear();
          _constFields.clear();
          _constStructFields.clear();
          _numRows = 0;
          _tableFlags = tagbyte & 0xF0;

          listener.onTableStart(tagbyte & 0xF0);
/*
          if (_isDecoratorTable && _decoratorListener._rows.size() > 0) {
            for (auto tupl : _decoratorListener._rows[0]) {
              _decoratorValues.push_back(tupl.second);
//              _decoratorValues[_decoratorValues.size()-1] += DECORATOR_INDEX_OFFSET;
            }
          }

          _isDecoratorTable = (tagbyte & TABLE_FLAG_DECORATE);

          if (_isDecoratorTable) {
            _decoratorFields.clear();
            _decoratorListener = GenericDecoderListener();
            _decoratorValues.clear();
          }
*/
        } else if (tagid == THFIELD) {

          SPCFieldInfo pField = _decodeFieldInfo(data, tagbyte);
          if (pField == 0L) {
            _markError(ENOSPC, data); return true;
          }

/*
          if (_isDecoratorTable) {
            // copy field def to decorator defs
            _decoratorFields.push_back(_fields[_fields.size()-1]);
            _decoratorFields[_decoratorFields.size()-1]->index += DECORATOR_INDEX_OFFSET;
          }
*/
        } else {
          _markError(EINVAL, data); return true;
        }

      }
      return false;
    }

    /**
     * Will decode fields, only to skip over it.
     * Will call listener.onRowStart() and listener.onRowEnd() only.
     */
    bool _doSkipRow(DecoderListener &listener, PData &data) {
      while (true) {

        uint8_t tagbyte;
        if (data.empty()) { _markError(ENOSPC, data); return true; }
        tagbyte = *data.ptr++;
        bool isIndex = (tagbyte & (uint8_t)0x80) != 0;
        uint8_t tagid = tagbyte & 0x0F;

        if (isIndex) {

          uint8_t index = tagbyte & (uint8_t)0x7F;
          if (index >= _fields.size()) {
            _markError(EINVAL, data); return true;
          }
          SPFieldInfo pField = _fields[index];

          if (_decodeValue(pField, data, listener)) {
            break;
          }

        } else if (tagid == TROW) {
          listener.onRowEnd((_numRows == 0),_data.start + _rowStartPos, (size_t)(_data.getOffset() - _rowStartPos - 1));

          _flags = (tagbyte >> 4) & 0x07;
          _rowStartPos = data.getOffset();
          { listener.onRowStart(); }
          if (_structLen > 0) {
            //auto structPtr = data.ptr;
            if (data.remaining() < _structLen) {
              throw new std::runtime_error("no more data, trying to read struct");
            }
            data.ptr += _structLen;
            // read struct
            size_t varlen = 0;
            if (_fields.size() > _structFields.size()) {
              // read length of variable length section
              varlen = readVarInt(data);
              if (varlen > data.remaining()) {
                throw new std::runtime_error("length of variable fields extends past end");
              }
            }

            //int rv = listener.onStruct(structPtr, _structLen, _constStructFields);
            if (true) {//rv == RV_SKIP_VARIABLE_FIELDS) {
              data.ptr += varlen;
            }
          }

          break;

        } else if (tagid == TFLAGS) {

          _flags = (tagbyte >> 4) & 0x07;

        } else if (tagid == TBLOCK) {

          // TODO

        } else if (tagid == TTABLE) {


          // TODO: snapshot decorators
          // clear previous table state.
          _structFields.clear();
          _fields.clear();
          _constFields.clear();
          _constStructFields.clear();
          _numRows = 0;
          _tableFlags = tagbyte & 0xF0;

          listener.onTableStart(tagbyte & 0xF0);

        } else if (tagid == THFIELD) {

          SPCFieldInfo pField = _decodeFieldInfo(data, tagbyte);
          if (pField == 0L) {
            _markError(ENOSPC, data); return true;
          }

        } else {
          _markError(EINVAL, data); return true;
        }

      }
      return false;
    }

    SPCFieldInfo _decodeFieldInfo(PData &data, uint8_t tagbyte) {

      bool has_subid = (tagbyte & FIELDINFO_FLAG_HAS_SUBID) != 0;
      bool has_name = (tagbyte & FIELDINFO_FLAG_HAS_NAME) != 0;
      bool isRaw = (tagbyte & FIELDINFO_FLAG_RAW) != 0;

      if (data.remaining() < 2) {
        _markError(ENOSPC, data);
        return 0L;
      }

      uint8_t index = *data.ptr++ & 0x7F;

      // indexes should decode in-order
      if (index != _fields.size()) {
        _markError(EINVAL, data);
        return 0L;
      }

      uint8_t typeId = *data.ptr++ & 0x0F;

      // sanity check typeId

      if (typeId == TNONE || typeId >= CrowType::NUM_TYPES) {
        _markError(EINVAL, data);
        throw new std::runtime_error("invalid typeId");
      }

      uint32_t id = readVarInt(data);
      uint32_t subid = 0;
      std::string name = std::string();

      // check for err
      if (_err > 0) return 0L;

      if (has_subid) {
        subid = readVarInt(data);
      }
      if (has_name) {
        size_t namelen = readVarInt(data);
        if (namelen > MAX_NAME_LEN) {
          _markError(EINVAL, data);
          return 0L;
        }
        // read name bytes
        name = std::string(reinterpret_cast<char const*>(data.ptr), namelen);
        data.ptr += namelen;
      }

      uint32_t fixedLen = 0;
      if (isRaw) {
        if ((CrowType)typeId == CrowType::TSTRING || (CrowType)typeId == CrowType::TBYTES) {
          fixedLen = readVarInt(data);
        } else {
          fixedLen = byte_size((CrowType)typeId);
        }
      }

      // TODO: check err

      SPSchemaId schema;
      if (has_subid) {
        schema = std::make_shared<SchemaId>("", subid);
      }
      const SPFieldDef fieldDef = FieldDef::alloc((DynType)typeId, name, id, schema);
      SPFieldInfo field = std::make_shared<FieldInfo>(fieldDef, index, fixedLen);
      _fields.push_back(field);
      _constFields.push_back(field);

      if (isRaw) {
        _structFields.push_back(field);
        _constStructFields.push_back(field);
        _structLen += fixedLen;
      }

      return field;
    }

    /**
     * returns 0 if no error, otherwise code from errno.h
     */
    int getErrCode() const override { return _err; }

    /**
     * returns bitwise OR of all TYPE_XX encountered in decoding.
     */
    uint64_t getTypeMask() const override { return _typemask; }

    virtual size_t getExpandedSize() override { return _byteCount; }

    virtual std::vector<SPCFieldInfo> getFields() override {
      auto tmp = std::vector<SPCFieldInfo>();
      for (auto f : _fields) { tmp.push_back(f); }
      /*
      if (_decoratorFields.size() > 0) {
        for (auto f : _decoratorFields) { tmp.push_back(f); }
      }*/
      return tmp;
    }

  private:

    bool _decodeValue(SPCFieldInfo pField, PData &data, DecoderListener &listener) {
      if (data.empty()) { _markError(ENOSPC, data); return true; }

      switch(pField->typeId) {
        case TINT32: {
          uint64_t tmp = readVarInt(data);
          int32_t val = ZigZagDecode32((uint32_t)tmp);
          if (NOT_SKIP_MODE) { listener.onField(pField, val, _flags); }
        }
        break;

        case TUINT32: {
          uint64_t tmp = readVarInt(data);
          uint32_t val = (uint32_t)tmp;
          if (NOT_SKIP_MODE) { listener.onField(pField, val, _flags); }
        }
        break;

        case TINT64: {
          uint64_t tmp = readVarInt(data);
          int64_t val = ZigZagDecode64(tmp);
          if (NOT_SKIP_MODE) { listener.onField(pField, val, _flags); }
        }
        break;

        case TUINT64: {
          uint64_t val = readVarInt(data);
          if (NOT_SKIP_MODE) { listener.onField(pField, val, _flags); }
        }
        break;

        case TFLOAT64: {
          uint64_t tmp = readFixed64(data);
          double val = DecodeDouble(tmp);
          if (NOT_SKIP_MODE) { listener.onField(pField, val, _flags); }
        }
        break;

        case TFLOAT32: {
          uint64_t tmp = readFixed32(data);
          double val = DecodeFloat(tmp);
          if (NOT_SKIP_MODE) { listener.onField(pField, val, _flags); }
        }
        break;

        case TUINT8: {
          uint8_t val = *data.ptr++;
          if (NOT_SKIP_MODE) { listener.onField(pField, val, _flags); }
        }
        break;

        case TINT8: {
          uint8_t val = *data.ptr++;
          if (NOT_SKIP_MODE) { listener.onField(pField, (int8_t)val, _flags); }
        }
        break;

        case TSTRING: {
          uint64_t len = readVarInt(data);
          if (data.remaining() < len) {
            _markError(ENOSPC, data);
            break;
          }
          if (NOT_SKIP_MODE) {
            std::string s(reinterpret_cast<char const*>(data.ptr), (size_t)len);
            listener.onField(pField, s, _flags);
          }
          data.ptr += len;
        }
        break;

        case TBYTES: {
          uint64_t len = readVarInt(data);
          if (data.remaining() < len) {
            _markError(ENOSPC, data);
            break;
          }
          auto vec = std::vector<uint8_t>();
          vec.resize(len);
          memcpy(vec.data(), data.ptr, (size_t)len);
          data.ptr += len;
          if (NOT_SKIP_MODE) { listener.onField(pField, vec, _flags); }
        }
        break;

        default:
          break;
      }
      return false;
    }

    const SetContext* _putSet(uint8_t setId, const uint8_t* ptr, size_t len)
    {
      auto pContext = new SetContext(setId, ptr, len);
      _mapSets[setId] = pContext;
      return pContext;
    }

    const SetContext* _getSet(uint8_t setId)
    {
      auto fit = _mapSets.find(setId);
      if (fit == _mapSets.end()) return 0L;
      return fit->second;
    }

    void _markError(int errCode, PData &data) {
      if (_err == 0) return;
      _err = errCode;
      _errOffset = (data.ptr - data.start);
      data.ptr = data.end;
    }

    uint64_t readFixed64(PData &data)
    {
      if (data.remaining() < sizeof(uint64_t)) {
        _markError(ENOSPC, data);
        return 0L;
      }
      uint64_t val = *((uint64_t*)data.ptr);
      data.ptr += sizeof(val);
      return val;
    }

    uint64_t readFixed32(PData &data)
    {
      if (data.remaining() < sizeof(uint32_t)) {
        _markError(ENOSPC, data);
        return 0L;
      }
      uint32_t val = *((uint32_t*)data.ptr);
      data.ptr += sizeof(val);
      return val;
    }

    //void setFieldIndexAdd(uint32_t value) override { _fieldIndexAdd = value; }

    PData _data;
    std::vector<SPFieldInfo> _fields;
    int            _err;
    uint64_t       _typemask;
    uint64_t       _errOffset;
    uint64_t       _setId;
    std::map<uint8_t, SetContext*> _mapSets;
    size_t         _byteCount;
    uint8_t        _flags;
    uint32_t       _numRows;
    std::vector<SPFieldInfo> _structFields;
    size_t         _structLen;
    size_t         _rowStartPos;
    int            _modeFlags;

    // these are duplicates of _fields and _structFields, marked as const for sharing with listeners
    std::vector<SPCFieldInfo> _constFields;
    std::vector<SPCFieldInfo> _constStructFields;
    uint8_t _tableFlags;

/*
    bool           _isDecoratorTable;
    std::vector<SPFieldInfo> _decoratorFields;
    GenericDecoderListener   _decoratorListener;
    std::vector<DecColValue> _decoratorValues;
*/
    uint64_t readVarInt(PData &data) {
      uint64_t value = 0L;
      uint64_t shift = 0L;

      while (!data.empty()) {
        uint8_t b = *data.ptr++;
        value |= ((uint64_t)(b & 0x07F)) << shift;
        shift += 7;
        if ((b >> 7) == 0) break;
      }

      return value;
    }

  };

  class DecoderFactory {
  public:
    static Decoder* New(const uint8_t* pEncData, size_t encLength) { return new DecoderImpl(pEncData, encLength); }
  };

}

#endif // _CROW_DECODE_IMPL_HPP_

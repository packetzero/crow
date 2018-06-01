#include <map>
#include <errno.h>

#include "../include/crow.hpp"
#include "stack.hpp"
#include "protobuf_wire_format.h"

#define NONE_LEFT(PTR) (PTR >= _end)
#define BYTES_REMAIN(PTR) (PTR < _end)
#define MAX_NAME_LEN 64
#define MAX_FIELDS   72    // ridiculously wide table

#define MAX_SANE_SET_SIZE 32000000 // 32 MB should be plenty

//#define TYPE_MASK_SET    (1U << TYPE_SET)
//#define TYPE_MASK_SETREF (1U << TYPE_SETREF)
//#define TYPE_MASK_ROWSEP (1U << TYPE_ROWSEP)

static const uint8_t UPPER_BIT = (uint8_t)0x80;

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
      _byteCount(encLength), _flags(0), _numRows(0) {
    }

    ~DecoderImpl() {
      auto it = _mapSets.begin();
      while (it != _mapSets.end()) {
        delete it->second;
        _mapSets.erase(it++);
      }
    }

    /*
     * decode
     */
    uint32_t decode(DecoderListener &listener, uint64_t setId = 0L) override {
      _setId = setId;

      _numRows = 0;
      while(false == decodeRow(listener)) {
        _numRows++;
      }

      return _numRows;
    }

/*
    void init(const uint8_t* pEncData, size_t encLength) override {
      _ptr = pEncData;
      _end = pEncData + encLength;
      _fields.clear();
    }
*/

    bool decodeRow(DecoderListener &listener) override {
      return _doDecodeRow(listener, _data);
    }

    bool _doDecodeRow(DecoderListener &listener, PData &data) {
      while (true) {

        uint8_t tagbyte;
        if (data.empty()) { _markError(ENOSPC, data); return true; }
        tagbyte = *data.ptr++;
        bool isIndex = (tagbyte & UPPER_BIT) != 0;
        uint8_t tagid = tagbyte & 0x0F;

        if (isIndex) {

          uint8_t index = tagid & (uint8_t)0x7F;
          if (index >= _fields.size()) {
            _markError(EINVAL, data); return true;
          }
          const Field* pField = &_fields[index];

          if (_decodeValue(pField, data, listener)) {
            break;
          }

        } else if (tagid == TROW) {
          if (_numRows > 0) { listener.onRowEnd(); }

          _flags = (tagbyte >> 4) & 0x07;
          break;

        } else if (tagid == TFLAGS) {

          _flags = (tagbyte >> 4) & 0x07;
/*
        } else if (tagid == TSET) {

          if (data.empty()) { _markError(ENOSPC, data); return true; }
          uint8_t setid = *data.ptr++;

          auto setlen = readVarInt(data);
          if (setlen == 0 || setlen > MAX_SANE_SET_SIZE) {
            _markError(ENOSPC, data); return true;
          }

          _putSet(setid, data.ptr, setlen);
          data.ptr += setlen;

        } else if (tagid == TSETREF) {

          uint8_t setId = *data.ptr++;
          const SetContext* pContext = _getSet(setId);
          if (pContext == 0L) {
            _markError(ESPIPE, data); return true;
          } else {
            _byteCount += pContext->size();
          }
          PData setData(pContext->data(), pContext->size());
          _doDecodeRow(listener, setData);
*/
        } else if (tagid == THFIELD) {

          //bool hasNoValue = (tagbyte & FIELDINFO_FLAG_NO_VALUE) == FIELDINFO_FLAG_NO_VALUE;
          const Field* pField = _decodeFieldInfo(data, tagbyte);
          if (pField == 0L) {
            _markError(ENOSPC, data); return true;
          }
          //if (hasNoValue || _decodeValue(pField, data, listener)) {
          //  break;
          //}
        } else {
          _markError(EINVAL, data); return true;
        }

      }
      return false;
    }

    const Field* _decodeFieldInfo(PData &data, uint8_t tagbyte) {

      bool has_subid = (tagbyte & FIELDINFO_FLAG_HAS_SUBID) != 0;
      bool has_name = (tagbyte & FIELDINFO_FLAG_HAS_NAME) != 0;

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

      // TODO : check valid typeid

      uint32_t id = readVarInt(data);
      uint32_t subid = 0;
      std::string name = std::string();

      // TODO: check err

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
      // TODO: check err

      _fields.push_back(Field());
      Field *pField = &_fields[index];
      pField->index = index;
      pField->typeId = (CrowType)typeId;
      pField->id = id;
      pField->subid = subid;
      pField->name = name;

      return pField;
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

    virtual std::vector<Field> getFields() override { return _fields; }

  private:

    bool _decodeValue(const Field *pField, PData &data, DecoderListener &listener) {
      if (data.empty()) { _markError(ENOSPC, data); return true; }

      switch(pField->typeId) {
        case TINT32: {
          uint64_t tmp = readVarInt(data);
          int32_t val = ZigZagDecode32((uint32_t)tmp);
          listener.onField(pField, val, _flags);
        }
        break;

        case TUINT32: {
          uint64_t tmp = readVarInt(data);
          uint32_t val = (uint32_t)tmp;
          listener.onField(pField, val, _flags);
        }
        break;

        case TINT64: {
          uint64_t tmp = readVarInt(data);
          int64_t val = ZigZagDecode64(tmp);
          listener.onField(pField, val, _flags);
        }
        break;

        case TUINT64: {
          uint64_t val = readVarInt(data);
          listener.onField(pField, val, _flags);
        }
        break;

        case TFLOAT64: {
          uint64_t tmp = readFixed64(data);
          double val = DecodeDouble(tmp);
          listener.onField(pField, val, _flags);
        }
        break;

        case TFLOAT32: {
          uint64_t tmp = readFixed32(data);
          double val = DecodeFloat(tmp);
          listener.onField(pField, val, _flags);
        }
        break;

        case TUINT8: {
          uint8_t val = *data.ptr++;
          listener.onField(pField, val, _flags);
        }
        break;

        case TINT8: {
          uint8_t val = *data.ptr++;
          listener.onField(pField, (int8_t)val, _flags);
        }
        break;

        case TSTRING: {
          uint64_t len = readVarInt(data);
          if (data.remaining() < len) {
            _markError(ENOSPC, data);
            break;
          }
          std::string s(reinterpret_cast<char const*>(data.ptr), (size_t)len);
          data.ptr += len;
          listener.onField(pField, s, _flags);
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
          listener.onField(pField, vec, _flags);
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
    std::vector<Field> _fields;
    int            _err;
    uint64_t       _typemask;
    uint64_t       _errOffset;
    uint64_t       _setId;
    std::map<uint8_t, SetContext*> _mapSets;
    size_t         _byteCount;
    uint8_t        _flags;
    uint32_t       _numRows;

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

  Decoder* DecoderNew(const uint8_t* pEncData, size_t encLength) { return new DecoderImpl(pEncData, encLength); }

}

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

#define TYPE_MASK_SET    (1U << TYPE_SET)
#define TYPE_MASK_SETREF (1U << TYPE_SETREF)
#define TYPE_MASK_ROWSEP (1U << TYPE_ROWSEP)

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


  /*
   * Implementation of Decoder
   */
  class DecoderImpl : public Decoder {
  public:
    /*
     * DecoderImpl constructor
     */
    DecoderImpl(const uint8_t* pEncData, size_t encLength) : Decoder(), _ptr(pEncData),
      _end(pEncData + encLength), _fields(), _err(0), _typemask(0L),
      _errOffset(0L), _setId(0L), _mapSets(), _fieldIndexAdd(0), _byteCount(encLength) {
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

      uint32_t numRows = 0;
      while(false == decodeRow(listener)) {
        numRows++;
      }

      return numRows;
    }

/*
    void init(const uint8_t* pEncData, size_t encLength) override {
      _ptr = pEncData;
      _end = pEncData + encLength;
      _fields.clear();
    }
*/

    bool decodeRow(DecoderListener &listener) override {

      while (true) {

        uint8_t tagid;
        if (NONE_LEFT(_ptr)) { _markError(ENOSPC, _ptr); return true; }
        tagid = *_ptr++;

        if ((tagid & UPPER_BIT) != 0) {

          uint8_t index = tagid & (uint8_t)0x7F;
          if (index >= _fields.size()) {
            _markError(EINVAL, _ptr); return true;
          }
          const Field* pField = &_fields[index];

          if (_decodeValue(pField, _ptr, listener)) {
            break;
          }

        } else if (tagid == TROWSEP) {
          listener.onRowSep();
          break;
        } else if (tagid == TFIELDINFO) {
          const Field* pField = _decodeFieldInfo(_ptr);
          if (pField == 0L) {
            _markError(ENOSPC, _ptr); return true;
          }
          if (_decodeValue(pField, _ptr, listener)) {
            break;
          }
        } else {
          _markError(EINVAL, _ptr); return true;
        }


        // TODO: field modifiers for sets
        //if (_fieldIndexAdd != 0) {
        //  fieldIndex |= _fieldIndexAdd;
        //}
      }
      return false;
    }

    const Field* _decodeFieldInfo(const uint8_t* &ptr) {
      if ((_end-ptr) < 2) {
        _markError(ENOSPC, ptr);
        return 0L;
      }
      uint8_t index = *ptr++;
      bool has_subid = (index & UPPER_BIT) != 0;
      index = index & 0x7F;

      // indexes should decode in-order
      if (index != _fields.size()) {
        _markError(EINVAL, ptr);
        return 0L;
      }

      uint8_t typeId = *ptr++;
      bool has_name = (typeId & UPPER_BIT) != 0;
      typeId = typeId & 0x7F;

      // TODO : check valid typeid

      uint32_t id = readVarInt(ptr);
      uint32_t subid = 0;
      std::string name = std::string();

      // TODO: check err

      if (has_subid) {
        subid = readVarInt(ptr);
      }
      if (has_name) {
        size_t namelen = readVarInt(ptr);
        if (namelen > MAX_NAME_LEN) {
          _markError(EINVAL, ptr);
          return 0L;
        }
        // read name bytes
        name = std::string(reinterpret_cast<char const*>(ptr), namelen);
        ptr += namelen;
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

  private:

    bool _decodeValue(const Field *pField, const uint8_t* &ptr, DecoderListener &listener) {
      if (NONE_LEFT(ptr)) { _markError(ENOSPC, ptr); return true; }

      switch(pField->typeId) {
        case TINT32: {
          uint64_t tmp = readVarInt(ptr);
          int32_t val = ZigZagDecode32((uint32_t)tmp);
          listener.onField(pField, val);
        }
        break;

        case TUINT32: {
          uint64_t tmp = readVarInt(ptr);
          uint32_t val = (uint32_t)tmp;
          listener.onField(pField, val);
        }
        break;

        case TINT64: {
          uint64_t tmp = readVarInt(ptr);
          int64_t val = ZigZagDecode64(tmp);
          listener.onField(pField, val);
        }
        break;

        case TUINT64: {
          uint64_t val = readVarInt(ptr);
          listener.onField(pField, val);
        }
        break;

        case TFLOAT64: {
          uint64_t tmp = readFixed64(ptr);
          double val = DecodeDouble(tmp);
          listener.onField(pField, val);
        }
        break;

        case TFLOAT32: {
          uint64_t tmp = readFixed32(ptr);
          double val = DecodeFloat(tmp);
          listener.onField(pField, val);
        }
        break;

        case TUINT8: {
          uint8_t val = *ptr++;
          listener.onField(pField, val);
        }
        break;

        case TINT8: {
          uint8_t val = *ptr++;
          listener.onField(pField, (int8_t)val);
        }
        break;

        case TSTRING: {
          uint64_t len = readVarInt(ptr);
          if ((_end-ptr) < len) {
            _markError(ENOSPC, ptr);
            break;
          }
          std::string s(reinterpret_cast<char const*>(ptr), (size_t)len);
          ptr += len;
          listener.onField(pField, s);
        }
        break;

        case TBYTES: {
          uint64_t len = readVarInt(ptr);
          if ((_end-ptr) < len) {
            _markError(ENOSPC, ptr);
            break;
          }
          auto vec = std::vector<uint8_t>();
          vec.resize(len);
          memcpy(vec.data(), ptr, (size_t)len);
          ptr += len;
          listener.onField(pField, vec);
        }
        break;
/*
        case TYPE_SET: {
          uint64_t setId = readVarInt(ptr);
          uint64_t len = readVarInt(ptr);
          if ((_end-ptr) < len) {
            _markError(ENOSPC, ptr);
            break;
          }
          const SetContext* pContext = _putSet(setId, ptr, len);
          ptr += len;
          if (pContext != 0L) {
            auto setDecoder = DecoderImpl(pContext->data(), pContext->size());
            setDecoder.setFieldIndexAdd(fieldIndex);
            setDecoder.decode(listener, setId);
          }
        }
        break;

        case TYPE_SETREF: {
          uint64_t setId = readVarInt(ptr);
          const SetContext* pContext = _getSet(setId);
          if (pContext == 0L) {
            _markError(ESPIPE, ptr);
          } else {
            _byteCount += pContext->size();
            auto setDecoder = DecoderImpl(pContext->data(), pContext->size());
            setDecoder.setFieldIndexAdd(fieldIndex);
            setDecoder.decode(listener, setId);
          }
        }
        break;
*/
        default:
          break;
      }
      return false;
    }
/*
    const SetContext* _putSet(uint64_t setId, const uint8_t* ptr, size_t len)
    {
      auto pContext = new SetContext(setId, ptr, len);
      _mapSets[setId] = pContext;
      return pContext;
    }

    const SetContext* _getSet(uint64_t setId)
    {
      auto fit = _mapSets.find(setId);
      if (fit == _mapSets.end()) return 0L;
      return fit->second;
    }
*/
    void _markError(int errCode, const uint8_t* ptr) {
      if (_err == 0) return;
      _err = errCode;
      _errOffset = (ptr - _ptr);
      ptr = _end;
    }

    uint64_t readFixed64(const uint8_t* &ptr)
    {
      if ((_end - ptr) < sizeof(uint64_t)) {
        _markError(ENOSPC, ptr);
        return 0L;
      }
      uint64_t val = *((uint64_t*)ptr);
      ptr += sizeof(val);
      return val;
    }

    uint64_t readFixed32(const uint8_t* &ptr)
    {
      if ((_end - ptr) < sizeof(uint32_t)) {
        _markError(ENOSPC, ptr);
        return 0L;
      }
      uint32_t val = *((uint32_t*)ptr);
      ptr += sizeof(val);
      return val;
    }

    //void setFieldIndexAdd(uint32_t value) override { _fieldIndexAdd = value; }

    const uint8_t* _ptr;
    const uint8_t* _end;
    std::vector<Field> _fields;
    int            _err;
    uint64_t       _typemask;
    uint64_t       _errOffset;
    uint64_t       _setId;
    std::map<uint64_t, SetContext*> _mapSets;
    uint32_t       _fieldIndexAdd;
    size_t         _byteCount;

    uint64_t readVarInt(const uint8_t* &ptr) {
      uint64_t value = 0L;
      uint64_t shift = 0L;

      while (BYTES_REMAIN(ptr)) {
        uint8_t b = *ptr++;
        value |= ((uint64_t)(b & 0x07F)) << shift;
        shift += 7;
        if ((b >> 7) == 0) break;
      }

      return value;
    }

  };

  Decoder* DecoderNew(const uint8_t* pEncData, size_t encLength) { return new DecoderImpl(pEncData, encLength); }

  class EncoderImpl : public Encoder {
  public:
    EncoderImpl(size_t initialCapacity) : Encoder(), _stack(initialCapacity), _hasHeaders(false), _setIdCounter(1), _fields() {}
    ~EncoderImpl() { }

    Field *fieldFor(CrowType type, uint32_t id, uint32_t subid = 0) override {
      for (int i=0; i < _fields.size(); i++) {
        Field *pField = &_fields[i];
        if (pField->id == id && pField->subid == subid) return pField;
      }
      Field* p = _newField(type);
      if (p == _dummyField()) return p;

      p->id = id;
      p->subid = subid;

      return p;
    }

    Field *fieldFor(CrowType type, std::string name) override {
      for (int i=0; i < _fields.size(); i++) {
        Field *pField = &_fields[i];
        if (pField->name == name) return pField;
      }
      Field* p = _newField(type);
      if (p == _dummyField()) return p;

      p->name = name;

      return p;
    }

    Field* _dummyField() {
      static Field dummy = { NONE /* type */, 0 /* id */, 9999999, "FIELD_LIMIT_REACHED", 0 /* index */ };
      return &dummy;
    }

    void put(const Field *pField, int32_t value) override {
      if (pField == _dummyField()) return;
      writeIndexTag(pField);
      writeVarInt(ZigZagEncode32(value));
    }
    void put(const Field *pField, uint32_t value) override {
      if (pField == _dummyField()) return;
      writeIndexTag(pField);
      writeVarInt(value);
    }
    void put(const Field *pField, int64_t value) override {
      if (pField == _dummyField()) return;
      writeIndexTag(pField);
      writeVarInt(ZigZagEncode64(value));
    }
    void put(const Field *pField, uint64_t value) override {
      if (pField == _dummyField()) return;
      writeIndexTag(pField);
      writeVarInt(value);
    }
    void put(const Field *pField, double value) override {
      if (pField == _dummyField()) return;
      writeIndexTag(pField);
      writeFixed64(EncodeDouble(value));
    }
    void put(const Field *pField, float value) override {
      if (pField == _dummyField()) return;
      writeIndexTag(pField);
      writeFixed32(EncodeFloat(value));
    }
    void put(const Field *pField, const std::string value) override {
      if (pField == _dummyField()) return;
      writeIndexTag(pField);
      writeVarInt(value.length());
      memcpy(_stack.Push(value.length()), value.c_str(), value.length());
    }
    void put(const Field *pField, const char* str) override {
      if (pField == _dummyField()) return;
      writeIndexTag(pField);
      size_t srcLen = strlen(str);
      writeVarInt(srcLen);
      memcpy(_stack.Push(srcLen), str, srcLen);
    }
    void put(const Field *pField, const std::vector<uint8_t>& value) override {
      if (pField == _dummyField()) return;
      writeIndexTag(pField);
      writeVarInt(value.size());
      memcpy(_stack.Push(value.size()), value.data(), value.size());
    }
    void put(const Field *pField, const uint8_t* bytes, size_t len) override {
      if (pField == _dummyField()) return;
      writeIndexTag(pField);
      writeVarInt(len);
      memcpy(_stack.Push(len), bytes, len);
    }
    void put(const Field *pField, bool value) override {
      put(pField, (value ? (uint8_t)1 : (uint8_t)0));
    }

    virtual void put(const Field *pField, int8_t value) override {
      if (pField == _dummyField()) return;
      writeIndexTag(pField);
      uint8_t* ptr = _stack.Push(1);
      *ptr = (uint8_t)value;
    }
    virtual void put(const Field *pField, uint8_t value) override {
      if (pField == _dummyField()) return;
      writeIndexTag(pField);
      uint8_t* ptr = _stack.Push(1);
      *ptr = value;
    }
    virtual void put(const Field *pField, int16_t value) override {
      if (pField == _dummyField()) return;
      writeIndexTag(pField);
      writeVarInt(ZigZagEncode32(value));
    }
    virtual void put(const Field *pField, uint16_t value) override {
      if (pField == _dummyField()) return;
      writeIndexTag(pField);
      writeVarInt(value);
    }

    void putRowSep() override {
      *(_stack.Push(1)) = TROWSEP;
    }

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
     * @param setId An ID used by encoding application for get/put.
     *
     * @return true on error, false on success.
     */
     /*
    bool putSet(uint64_t appSetId, Encoder& encForSet, uint32_t fieldIndexAdd) override {

      // sanity check to make sure we haven't already encountered setId

      auto fit = _mapSets.find(appSetId);
      if (fit != _mapSets.end()) return true;

      if (encForSet.size() == 0 || encForSet.size() > MAX_SANE_SET_SIZE) return true;

      uint64_t setId = _setIdCounter++;

      writeTag(fieldIndexAdd, TYPE_SET);
      writeVarInt(setId);
      writeVarInt(encForSet.size());

      // Make sure data is valid and contains no SET or SETREF or ROWSEP

      DecoderImpl decTest = DecoderImpl(encForSet.data(), encForSet.size());
      auto emptyListener = DecoderListener();
      decTest.decode(emptyListener);
      if (decTest.getErrCode() != 0 || (decTest.getTypeMask() & (TYPE_MASK_SET | TYPE_MASK_SETREF | TYPE_MASK_ROWSEP))) {
        return true;
      }

      // place data

      memcpy(_stack.Push(encForSet.size()), encForSet.data(), encForSet.size());

      // update map

      _mapSets[appSetId] = setId;

      return false;
    }*/

    /**
     * @brief If found (previous call to putSet(setId) was made), places SETREF.
     *
     * @returns true on error, false on success.
     */
     /*
    bool putSetRef(uint64_t appSetId, uint32_t fieldIndexAdd) override {
      auto fit = _mapSets.find(appSetId);
      if (fit == _mapSets.end()) return true;

      writeTag(fieldIndexAdd, TYPE_SETREF);
      writeVarInt(fit->second);

      return false;
    }*/

    const uint8_t* data() const override { return _stack.Bottom(); }

    size_t size() const override { return _stack.GetSize(); }

    void clear() override { _stack.Clear(); _hasHeaders = false; _setIdCounter = 1;}

  private:

    Field *_newField(CrowType typeId) {
      // new one
      uint8_t index = (uint8_t)_fields.size();
      if ((index & UPPER_BIT) != 0) {
        // wow, 127 fields?  return a dummy field .. one that doesn't get written
        return _dummyField();
      }
      _fields.push_back(Field());
      Field *p = &_fields[index];
      p->index = index;
      p->typeId = typeId;
      return p;
    }

    void writeFixed64(uint64_t value) {
      union { uint64_t ival; uint8_t bytes[8];};
      ival = value;
      memcpy(_stack.Push(sizeof(value)), bytes, sizeof(bytes));
    }
    void writeFixed32(uint32_t value) {
      union { uint32_t ival; uint8_t bytes[4];};
      ival = value;
      memcpy(_stack.Push(sizeof(value)), bytes, sizeof(bytes));
    }
    /*
     * one byte 0x80 | index
     */
    void writeIndexTag(const Field *pField) {
      if (pField->_written) {

        uint8_t* ptr = _stack.Push(1);
        ptr[0] = pField->index | UPPER_BIT;

      } else {

        // index
        uint8_t* ptr = _stack.Push(2);
        ptr[0] = CrowTag::TFIELDINFO;
        ptr++;
        if (pField->subid > 0) {
          ptr[0] = pField->index | UPPER_BIT;
        } else {
          ptr[0] = pField->index;
        }

        // typeid
        ptr = _stack.Push(1);
        size_t namelen = pField->name.length();
        if (namelen > 0) {
          ptr[0] = pField->typeId | UPPER_BIT;
        } else {
          ptr[0] = pField->typeId;
        }

        // id and subid (if set)
        writeVarInt(pField->id);
        if (pField->subid > 0) { writeVarInt(pField->subid); }

        // name (if set)
        if (namelen > 0) {
          writeVarInt(namelen);
          ptr = _stack.Push(namelen);
          memcpy(ptr,pField->name.c_str(),namelen);
        }

        // mark as written, so we dont write FIELDINFO more than once
        ((Field*)pField)->_written = true;
      }
    }

    size_t writeVarInt(uint64_t value) {
      size_t i=0;
      while (true) {
        i++;
        uint8_t b = (uint8_t)(value & 0x07F);
        value = value >> 7;
        if (0L == value) {
          uint8_t* ptr = _stack.Push(1);
          *ptr = b;
          break;
        }
        uint8_t* p = _stack.Push(1);
        *p = (b | 0x80u);
      }

      return i;
    }

    Stack _stack;
    bool _hasHeaders;
    std::map<uint64_t,uint64_t> _mapSets;
    uint32_t _setIdCounter;
    std::vector<Field> _fields;
  };

  Encoder* EncoderNew(size_t initialCapacity) { return new EncoderImpl(initialCapacity); }

}

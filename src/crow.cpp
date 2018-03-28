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

    void init(const uint8_t* pEncData, size_t encLength) override {
      _ptr = pEncData;
      _end = pEncData + encLength;
      memset(_fields, 0, sizeof(_fields)); // empty out field pointers
    }

    /**
     * @brief Scans for next RowSep tag, returns true if found.
     *   Keeps track of bytes for entire row, for use by subsequent
     *   calls to decodeField().
     */
    bool scanRow() override {
      if (moveAfterRowSep()) {
        // error or end of buffer
        return false;
      }
      return true;
    }

    /**
     * @brief After a call to scanRow()==false, decodeField() can be used
     *  to decode the first entry matching fieldIndex.
     * @returns true if found, false otherwise.
     */
    bool decodeField(uint32_t requestedFieldIndex, DecoderListener& listener) override {
      if (requestedFieldIndex >= MAX_FIELDS) return false;

      auto fieldPtr = _fields[requestedFieldIndex];

      if (fieldPtr == nullptr) return false;

      uint32_t fieldIndex;
      FieldType fieldType;
      std::string columnName = std::string();
      if (readTag(fieldIndex, fieldType, columnName, fieldPtr)) {
        // failed or end of buffer
        return false;
      }
      if (requestedFieldIndex != fieldIndex) return false;

      _decodeField(fieldIndex, fieldType, columnName, listener, fieldPtr);

      return true;
    }

    bool decodeRow(DecoderListener &listener) override {

      while (true) {

        uint32_t fieldIndex;
        FieldType fieldType;
        std::string columnName = std::string();
        if (readTag(fieldIndex, fieldType, columnName, _ptr)) {
          // failed or end of buffer
          return true;
        }

        if (_fieldIndexAdd != 0) {
          fieldIndex |= _fieldIndexAdd;
        }
        
        if (_decodeField(fieldIndex, fieldType, columnName, listener, _ptr)) {
          break;
        }
      }
      return false;
    }

    uint32_t getLastFieldIndex() override {
      for (int i=(MAX_FIELDS-1); i >= 0; i--) {
        if (_fields[i] != nullptr) return i;
      }
      return 0;
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

    bool _decodeField(uint32_t fieldIndex, FieldType fieldType, std::string &columnName, DecoderListener &listener, const uint8_t* &ptr) {
      _typemask |= (1UL << fieldType);
  
      switch(fieldType) {
        case TYPE_INT32: {
          uint64_t tmp = readVarInt(ptr);
          int32_t val = ZigZagDecode32((uint32_t)tmp);
          listener.onField(fieldIndex, val, columnName, _setId);
        }
        break;

        case TYPE_UINT32: {
          uint64_t tmp = readVarInt(ptr);
          uint32_t val = (uint32_t)tmp;
          listener.onField(fieldIndex, val, columnName, _setId);
        }
        break;

        case TYPE_INT64: {
          uint64_t tmp = readVarInt(ptr);
          int64_t val = ZigZagDecode64(tmp);
          listener.onField(fieldIndex, val, columnName, _setId);
        }
        break;

        case TYPE_UINT64: {
          uint64_t val = readVarInt(ptr);
          listener.onField(fieldIndex, val, columnName, _setId);
        }
        break;

        case TYPE_DOUBLE: {
          uint64_t tmp = readFixed64(ptr);
          double val = DecodeDouble(tmp);
          listener.onField(fieldIndex, val, columnName, _setId);
        }
        break;

        case TYPE_BOOL: {
          uint64_t tmp = readVarInt(ptr);
          listener.onField(fieldIndex, (bool)tmp, columnName, _setId);
        }
        break;

        case TYPE_ROWSEP: {
          listener.onRowSep();
          return false;
        }
        break;

        case TYPE_STRING: {
          uint64_t len = readVarInt(ptr);
          if ((_end-ptr) < len) {
            _markError(ENOSPC, ptr);
            break;
          }
          std::string s(reinterpret_cast<char const*>(ptr), (size_t)len);
          ptr += len;
          listener.onField(fieldIndex, s, columnName, _setId);
        }
        break;

        case TYPE_BYTES: {
          uint64_t len = readVarInt(ptr);
          if ((_end-ptr) < len) {
            _markError(ENOSPC, ptr);
            break;
          }
          auto vec = std::vector<uint8_t>();
          vec.resize(len);
          memcpy(vec.data(), ptr, (size_t)len);
          ptr += len;
          listener.onField(fieldIndex, vec, columnName, _setId);
        }
        break;

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

        default:
          break;
      }
      return false;
    }

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
      ptr += 8;
      return val;
    }

    void setFieldIndexAdd(uint32_t value) override { _fieldIndexAdd = value; }
    
    /*
     * Reads (varint-length, bytes) of a string at *ptr, but before *end.
     * If pColumnName is not nullptr, it will be initialized with string,
     * otherwise, the bytes are just skipped.
     * The ptr is updated to point to immediately after the string.
     */
    inline void readColumnName(std::string *pColumnName, const uint8_t* &ptr, const uint8_t* &end)
    {
      auto len = readVarInt(ptr);
      if (len == 0 || len >= MAX_NAME_LEN) {
        return;
      } else {
        if ((end-ptr) < len) {
          _markError(ENOSPC, ptr);
        } else {
          if (nullptr != pColumnName) {
            *pColumnName = std::string(reinterpret_cast<char const*>(ptr), (size_t)len);
          }
          ptr += len;
        }
      }
    }

    /*
     * Tags are always 2 bytes (fieldIndex, FieldType)
     * If encoded FieldType has bit 7 set, columnName will be set.
     */
    bool readTag(uint32_t &fieldIndex, FieldType &ft, std::string &columnName, const uint8_t* &ptr) {
      if (NONE_LEFT(ptr)) { _markError(ENOSPC, ptr); return true; }
      fieldIndex = *ptr++;
      if (NONE_LEFT(ptr)) { ft = (FieldType)0; _markError(ENOSPC, ptr); return true; }
      ft = (FieldType) *ptr++;

      if (ft & 0x80) {
        ft = (FieldType)(ft & 0x7F);
        readColumnName(&columnName, ptr, _end);
      }
      return false;
    }

    bool _skipFieldBytes(FieldType fieldType, const uint8_t* &ptr) {
      switch(fieldType) {
        case TYPE_UINT32:
        case TYPE_INT32:
        case TYPE_INT64:
        case TYPE_UINT64:
        case TYPE_BOOL:
          readVarInt(ptr);
          break;
        case TYPE_DOUBLE:
          ptr += sizeof(uint64_t);
          break;
        case TYPE_STRING: {
          uint64_t len = readVarInt(ptr);
          if ((_end-ptr) < len) {
            _markError(ENOSPC, ptr);
            break;
          }
          ptr += len;
        }
          break;

        case TYPE_BYTES: {
          uint64_t len = readVarInt(ptr);
          if ((_end-ptr) < len) {
            _markError(ENOSPC, ptr);
            break;
          }
          ptr += len;
        }
          break;

        default:
          break;
      }
      return false;
    }


    /*
     * Returns false on success, true otherwise.
     */
    bool moveAfterRowSep() {
      memset(_fields, 0, sizeof(_fields)); // empty out field pointers

      while (true) {
        if (NONE_LEFT(_ptr)) { _markError(ENOSPC, _ptr); return true; }
        auto fieldIndex = *_ptr++;
        if (NONE_LEFT(_ptr)) { _markError(ENOSPC, _ptr); return true; }
        auto ft = (FieldType) *_ptr++;

        if (ft == TYPE_ROWSEP) {
          return false;  // success
        }

        if (fieldIndex < MAX_FIELDS) {
          _fields[fieldIndex] = (_ptr - 2); // point to start (fieldIndex tag)
        }

        if (ft & 0x80) {
          // the column name precedes value
          ft = (FieldType)(ft & 0x7F);
          auto len = readVarInt(_ptr);
          if (len == 0 || len >= MAX_NAME_LEN) {
            _markError(ENOSPC, _ptr);
            return true;
          } else {
            if ((_end-_ptr) < len) {
              _markError(ENOSPC, _ptr);
              return true;
            } else {
              _ptr += len;
            }
          }
        }

        _skipFieldBytes(ft, _ptr);
      }
      return true;
    }

    const uint8_t* _ptr;
    const uint8_t* _end;
    const uint8_t* _fields[MAX_FIELDS];
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
    EncoderImpl(size_t initialCapacity) : Encoder(), _stack(initialCapacity), _hasHeaders(false), _setIdCounter(1) {}
    ~EncoderImpl() { }

    void put(uint32_t fieldIndex, int32_t value, const std::string name) override {
      writeTag(fieldIndex, TYPE_INT32, name);
      writeVarInt(ZigZagEncode32(value));
    }
    void put(uint32_t fieldIndex, uint32_t value, const std::string name) override {
      writeTag(fieldIndex, TYPE_UINT32, name);
      writeVarInt(value);
    }
    void put(uint32_t fieldIndex, int64_t value, const std::string name) override {
      writeTag(fieldIndex, TYPE_INT64, name);
      writeVarInt(ZigZagEncode64(value));

    }
    void put(uint32_t fieldIndex, uint64_t value, const std::string name) override {
      writeTag(fieldIndex, TYPE_UINT64, name);
      writeVarInt(value);
    }

    void put(uint32_t fieldIndex, double value, const std::string name) override {
      writeTag(fieldIndex, TYPE_DOUBLE, name);
      writeFixed64(EncodeDouble(value));
    }

    void put(uint32_t fieldIndex, bool value, const std::string name) override {
      writeTag(fieldIndex, TYPE_BOOL, name);
      writeVarInt((uint32_t)value);
    }

    void put(uint32_t fieldIndex, const char* str, const std::string name) override {
      size_t srcLen = strlen(str);
      writeTag(fieldIndex, TYPE_STRING, name);
      writeVarInt(srcLen);
      memcpy(_stack.Push(srcLen), str, srcLen);
    }

    void put(uint32_t fieldIndex, const std::string value, const std::string name) override {
      writeTag(fieldIndex, TYPE_STRING, name);
      writeVarInt(value.length());
      memcpy(_stack.Push(value.length()), value.c_str(), value.length());
    }

    void put(uint32_t fieldIndex, const uint8_t* bytes, size_t len, const std::string name) override {
      writeTag(fieldIndex, TYPE_BYTES, name);
      writeVarInt(len);
      memcpy(_stack.Push(len), bytes, len);
    }

    void put(uint32_t fieldIndex, const std::vector<uint8_t>& value, const std::string name) override {
      writeTag(fieldIndex, TYPE_BYTES, name);
      writeVarInt(value.size());
      memcpy(_stack.Push(value.size()), value.data(), value.size());
    }

    void putRowSep() override {
      writeTag(0, TYPE_ROWSEP);
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
    }

    /**
     * @brief If found (previous call to putSet(setId) was made), places SETREF.
     *
     * @returns true on error, false on success.
     */
    bool putSetRef(uint64_t appSetId, uint32_t fieldIndexAdd) override {
      auto fit = _mapSets.find(appSetId);
      if (fit == _mapSets.end()) return true;

      writeTag(fieldIndexAdd, TYPE_SETREF);
      writeVarInt(fit->second);

      return false;
    }

    const uint8_t* data() const override { return _stack.Bottom(); }

    size_t size() const override { return _stack.GetSize(); }

    void clear() override { _stack.Clear(); _hasHeaders = false; _setIdCounter = 1;}

  private:

    void writeFixed64(uint64_t value) {
      union { uint64_t ival; uint8_t bytes[8];};
      ival = value;
      memcpy(_stack.Push(sizeof(value)), bytes, sizeof(bytes));
    }

    /*
     * Tags are always 2 bytes (fieldIndex, FieldType)
     */
    void writeTag(uint32_t fieldIndex, FieldType ft, const std::string name = "") {
      uint8_t* ptr = _stack.Push(2);
      ptr[0] = (uint8_t)(fieldIndex & 0x7F);
      if (name.length() > 0) {
        // set bit 7
        ptr[1] = (uint8_t)((ft & 0x7F) | 0x080);
        // write length-delimited name characters
        writeVarInt(name.length());
        memcpy(_stack.Push(name.length()), name.c_str(), name.length());
      } else {
        ptr[1] = (uint8_t)(ft & 0x7F);
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
  };

  Encoder* EncoderNew(size_t initialCapacity) { return new EncoderImpl(initialCapacity); }

}

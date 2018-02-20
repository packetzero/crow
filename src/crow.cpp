#include "../include/crow.hpp"
#include "stack.hpp"
#include "protobuf_wire_format.h"

#define NONE_LEFT(PTR) (PTR >= _end)
#define BYTES_REMAIN(PTR) (PTR < _end)
#define MAX_NAME_LEN 64
#define MAX_FIELDS   72    // ridiculously wide table

namespace crow {

  class EncoderImpl : public Encoder {
  public:
    EncoderImpl(size_t initialCapacity) : Encoder(), _stack(initialCapacity), _hasHeaders(false) {}
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

    const uint8_t* data() const override { return _stack.Bottom(); }

    size_t size() const override { return _stack.GetSize(); }

    void clear() override { _stack.Clear(); _hasHeaders = false; }

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
  };

  Encoder* EncoderNew(size_t initialCapacity) { return new EncoderImpl(initialCapacity); }

  class DecoderImpl : public Decoder {
  public:
    DecoderImpl(const uint8_t* pEncData, size_t encLength) : Decoder(), _ptr(pEncData), _end(pEncData + encLength), _fields() {}
    ~DecoderImpl() {}

    uint32_t decode(DecoderListener &listener) override {

      uint32_t numRows = 0;
      while(false == decodeRow(listener)) {
        numRows++;
      }

      return numRows;
    }

    virtual void init(const uint8_t* pEncData, size_t encLength) {
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

        if (_decodeField(fieldIndex, fieldType, columnName, listener, _ptr)) {
          break;
        }
      }
      return false;
    }

    uint32_t getLastFieldIndex() override {
      for (uint32_t i=(MAX_FIELDS-1); i >= 0; i--) {
        if (_fields[i] != nullptr) return i;
      }
      return 0;
    }


  private:

    bool _decodeField(uint32_t fieldIndex, FieldType fieldType, std::string &columnName, DecoderListener &listener, const uint8_t* &ptr) {
      switch(fieldType) {
        case TYPE_INT32: {
          uint64_t tmp = readVarInt(ptr);
          int32_t val = ZigZagDecode32((uint32_t)tmp);
          listener.onField(fieldIndex, val, columnName);
        }
        break;

        case TYPE_UINT32: {
          uint64_t tmp = readVarInt(ptr);
          uint32_t val = (uint32_t)tmp;
          listener.onField(fieldIndex, val, columnName);
        }
        break;

        case TYPE_INT64: {
          uint64_t tmp = readVarInt(ptr);
          int64_t val = ZigZagDecode64(tmp);
          listener.onField(fieldIndex, val, columnName);
        }
        break;

        case TYPE_UINT64: {
          uint64_t val = readVarInt(ptr);
          listener.onField(fieldIndex, val, columnName);
        }
        break;

        case TYPE_DOUBLE: {
          uint64_t tmp = readFixed64(ptr);
          double val = DecodeDouble(tmp);
          listener.onField(fieldIndex, val, columnName);
        }
        break;

        case TYPE_BOOL: {
          uint64_t tmp = readVarInt(ptr);
          listener.onField(fieldIndex, (bool)tmp, columnName);
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
            // error
            ptr = _end;
            break;
          }
          std::string s(reinterpret_cast<char const*>(ptr), (size_t)len);
          ptr += len;
          listener.onField(fieldIndex, s, columnName);
        }
        break;

        case TYPE_BYTES: {
          uint64_t len = readVarInt(ptr);
          if ((_end-ptr) < len) {
            // error
            ptr = _end;
            break;
          }
          auto vec = std::vector<uint8_t>();
          vec.resize(len);
          memcpy(vec.data(), ptr, (size_t)len);
          ptr += len;
          listener.onField(fieldIndex, vec, columnName);
        }
        break;

        default:
          break;
      }
      return false;
    }

    uint64_t readFixed64(const uint8_t* &ptr)
    {
      if ((_end - ptr) < sizeof(uint64_t)) { ptr = _end; return 0L; }
      uint64_t val = *((uint64_t*)ptr);
      ptr += 8;
      return val;
    }

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
          // error
          ptr = end;
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
      if (NONE_LEFT(ptr)) { return true; }
      fieldIndex = *ptr++;
      if (NONE_LEFT(ptr)) { ft = (FieldType)0; return true; }
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
            // error
            ptr = _end;
            break;
          }
          ptr += len;
        }
          break;

        case TYPE_BYTES: {
          uint64_t len = readVarInt(ptr);
          if ((_end-ptr) < len) {
            // error
            ptr = _end;
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
        if (NONE_LEFT(_ptr)) { return true; }
        auto fieldIndex = *_ptr++;
        if (NONE_LEFT(_ptr)) { return true; }
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
            // TODO: log error
            _ptr = _end;
            return true;
          } else {
            if ((_end-_ptr) < len) {
              // error
              _ptr = _end;
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

}

#include "../include/crow.hpp"
#include "stack.hpp"

#define NONE_LEFT() (_ptr >= _end)
#define BYTES_REMAIN() (_ptr < _end)
#define MAX_NAME_LEN 64

//
// taken from protobuf wire_format_lite.h
inline uint32_t ZigZagEncode32(int32_t n) {
  // Note:  the right-shift must be arithmetic
  // Note:  left shift must be unsigned because of overflow
  return (static_cast<uint32_t>(n) << 1) ^ static_cast<uint32_t>(n >> 31);
}

inline int32_t ZigZagDecode32(uint32_t n) {
  // Note:  Using unsigned types prevent undefined behavior
  return static_cast<int32_t>((n >> 1) ^ (~(n & 1) + 1));
}

inline uint64_t ZigZagEncode64(int64_t n) {
  // Note:  the right-shift must be arithmetic
  // Note:  left shift must be unsigned because of overflow
  return (static_cast<uint64_t>(n) << 1) ^ static_cast<uint64_t>(n >> 63);
}

inline int64_t ZigZagDecode64(uint64_t n) {
  // Note:  Using unsigned types prevent undefined behavior
  return static_cast<int64_t>((n >> 1) ^ (~(n & 1) + 1));
}

inline uint64_t EncodeDouble(double value) {
  union {double f; uint64_t i;};
  f = value;
  return i;
}

inline double DecodeDouble(uint64_t value) {
  union {double f; uint64_t i;};
  i = value;
  return f;
}
// end protobuf wire_format_lite.h funcs

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
    DecoderImpl() : Decoder(), _ptr(0L), _end(0L) {}
    ~DecoderImpl() {}

    bool decode(const Bytes &vec, DecoderListener &listener) override {
      _ptr = vec.data();
      _end = vec.data() + vec.size();

      while(true) {
        uint32_t fieldIndex;
        FieldType fieldType;
        std::string columnName = std::string();
        if (readTag(fieldIndex, fieldType, columnName)) {
          // failed or end of buffer
          break;
        }
        switch(fieldType) {
          case TYPE_INT32: {
            uint64_t tmp = readVarInt();
            int32_t val = ZigZagDecode32((uint32_t)tmp);
            listener.onField(fieldIndex, val, columnName);
          }
          break;

          case TYPE_UINT32: {
            uint64_t tmp = readVarInt();
            uint32_t val = (uint32_t)tmp;
            listener.onField(fieldIndex, val, columnName);
          }
          break;

          case TYPE_INT64: {
            uint64_t tmp = readVarInt();
            int64_t val = ZigZagDecode64(tmp);
            listener.onField(fieldIndex, val, columnName);
          }
          break;

          case TYPE_UINT64: {
            uint64_t val = readVarInt();
            listener.onField(fieldIndex, val, columnName);
          }
          break;

          case TYPE_DOUBLE: {
            uint64_t tmp = readFixed64();
            double val = DecodeDouble(tmp);
            listener.onField(fieldIndex, val, columnName);
          }
          break;

          case TYPE_BOOL: {
            uint64_t tmp = readVarInt();
            listener.onField(fieldIndex, (bool)tmp, columnName);
          }
          break;

          case TYPE_ROWSEP: {
            listener.onRowSep();
          }
          break;

          case TYPE_STRING: {
            uint64_t len = readVarInt();
            if ((_end-_ptr) < len) {
              // error
              _ptr = _end;
              break;
            }
            std::string s(reinterpret_cast<char const*>(_ptr), (size_t)len);
            _ptr += len;
            listener.onField(fieldIndex, s, columnName);
          }
          break;

          case TYPE_BYTES: {
            uint64_t len = readVarInt();
            if ((_end-_ptr) < len) {
              // error
              _ptr = _end;
              break;
            }
            auto vec = std::vector<uint8_t>();
            vec.resize(len);
            memcpy(vec.data(), _ptr, (size_t)len);
            _ptr += len;
            listener.onField(fieldIndex, vec, columnName);
          }
          break;

          default:
            break;
        }
      }

      _ptr = 0L;
      _end = 0L;

      return false;
    }

  private:

    uint64_t readFixed64()
    {
      if ((_end - _ptr) < sizeof(uint64_t)) { _ptr = _end; return 0L; }
      uint64_t val = *((uint64_t*)_ptr);
      _ptr += 8;
      return val;
    }

    /*
     * Tags are always 2 bytes (fieldIndex, FieldType)
     */
    bool readTag(uint32_t &fieldIndex, FieldType &ft, std::string &columnName) {
      if (NONE_LEFT()) { return true; }
      fieldIndex = *_ptr++;
      if (NONE_LEFT()) { ft = (FieldType)0; return true; }
      ft = (FieldType) *_ptr++;

      if (ft & 0x80) {
        // the column name precedes value
        ft = (FieldType)(ft & 0x7F);
         auto len = readVarInt();
         if (len == 0 || len >= MAX_NAME_LEN) {
           // TODO: log error
         } else {
           if ((_end-_ptr) < len) {
             // error
             _ptr = _end;
           } else {
             columnName = std::string(reinterpret_cast<char const*>(_ptr), (size_t)len);
             _ptr += len;
           }
         }
      }
      return false;
    }

    const uint8_t* _ptr;
    const uint8_t* _end;

    uint64_t readVarInt() {
      uint64_t value = 0L;
      uint64_t shift = 0L;

      while (BYTES_REMAIN()) {
        uint8_t b = *_ptr++;
        value |= ((uint64_t)(b & 0x07F)) << shift;
        shift += 7;
        if ((b >> 7) == 0) break;
      }

      return value;
    }
  };

  Decoder* DecoderNew() { return new DecoderImpl(); }

}

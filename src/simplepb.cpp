#include "../include/simplepb.hpp"
#include "stack.hpp"

#define NONE_LEFT() (_ptr >= _end)
#define BYTES_REMAIN() (_ptr < _end)

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

namespace simplepb {

  class EncoderImpl : public Encoder {
  public:
    EncoderImpl(size_t initialCapacity) : Encoder(), _stack(initialCapacity), _hasHeaders(false) {}
    ~EncoderImpl() { }

    void putHeader(uint32_t fieldIndex, FieldType fieldType, const std::string name) override {
      if (!_hasHeaders) {
        writeTag(0, TYPE_ROWSEP);  // precede with ROWSEP if first row is header
      }
      _hasHeaders = true;
      writeTag(fieldIndex, fieldType);
      writeVarInt(name.length());
      memcpy(_stack.Push(name.length()), name.c_str(), name.length());
    }

    void put(uint32_t fieldIndex, int32_t value) override {
      writeTag(fieldIndex, TYPE_INT32);
      writeVarInt(ZigZagEncode32(value));
    }
    void put(uint32_t fieldIndex, uint32_t value) override {
      writeTag(fieldIndex, TYPE_UINT32);
      writeVarInt(value);
    }
    void put(uint32_t fieldIndex, int64_t value) override {
      writeTag(fieldIndex, TYPE_INT64);
      writeVarInt(ZigZagEncode64(value));

    }
    void put(uint32_t fieldIndex, uint64_t value) override {
      writeTag(fieldIndex, TYPE_UINT64);
      writeVarInt(value);
    }

    void put(uint32_t fieldIndex, double value) override {
      writeTag(fieldIndex, TYPE_DOUBLE);
      writeFixed64(EncodeDouble(value));
    }

    void put(uint32_t fieldIndex, bool value) override {
      writeTag(fieldIndex, TYPE_BOOL);
      writeVarInt((uint32_t)value);
    }

    void put(uint32_t fieldIndex, const char* str) override {
      size_t srcLen = strlen(str);
      writeTag(fieldIndex, TYPE_STRING);
      writeVarInt(srcLen);
      memcpy(_stack.Push(srcLen), str, srcLen);
    }

    void put(uint32_t fieldIndex, const std::string value) override {
      writeTag(fieldIndex, TYPE_STRING);
      writeVarInt(value.length());
      memcpy(_stack.Push(value.length()), value.c_str(), value.length());
    }

    void put(uint32_t fieldIndex, const uint8_t* bytes, size_t len) override {
      writeTag(fieldIndex, TYPE_BYTES);
      writeVarInt(len);
      memcpy(_stack.Push(len), bytes, len);
    }

    void put(uint32_t fieldIndex, const std::vector<uint8_t>& value) override {
      writeTag(fieldIndex, TYPE_BYTES);
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
    void writeTag(uint32_t fieldIndex, FieldType ft) {
      uint8_t* ptr = _stack.Push(2);
      ptr[0] = (uint8_t)(fieldIndex & 0x7F);
      ptr[1] = (uint8_t)(ft & 0x7F);
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
        if (readTag(fieldIndex, fieldType)) {
          // failed or end of buffer
          break;
        }
        switch(fieldType) {
          case TYPE_INT32: {
            uint64_t tmp = readVarInt();
            int32_t val = ZigZagDecode32((uint32_t)tmp);
            listener.onField(fieldIndex, val);
          }
          break;

          case TYPE_UINT32: {
            uint64_t tmp = readVarInt();
            uint32_t val = (uint32_t)tmp;
            listener.onField(fieldIndex, val);
          }
          break;

          case TYPE_INT64: {
            uint64_t tmp = readVarInt();
            int64_t val = ZigZagDecode64(tmp);
            listener.onField(fieldIndex, val);
          }
          break;

          case TYPE_UINT64: {
            uint64_t val = readVarInt();
            listener.onField(fieldIndex, val);
          }
          break;

          case TYPE_DOUBLE: {
            uint64_t tmp = readFixed64();
            double val = DecodeDouble(tmp);
            listener.onField(fieldIndex, val);
          }
          break;

          case TYPE_BOOL: {
            uint64_t tmp = readVarInt();
            listener.onField(fieldIndex, (bool)tmp);
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
            listener.onField(fieldIndex, s);
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
            listener.onField(fieldIndex, vec);
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
    bool readTag(uint32_t &fieldIndex, FieldType &ft) {
      if (NONE_LEFT()) { return true; }
      fieldIndex = *_ptr++;
      if (NONE_LEFT()) { ft = (FieldType)0; return true; }
      ft = (FieldType) *_ptr++;
      return false;
    }

    const uint8_t* _ptr;
    const uint8_t* _end;

    uint64_t readVarInt() { //uint64_t &value) {
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

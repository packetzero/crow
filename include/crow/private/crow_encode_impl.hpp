#ifndef _CROW_ENCODE_IMPL_HPP_
#define _CROW_ENCODE_IMPL_HPP_
#include <map>
#include <errno.h>
#include <stdexcept>
#include <unistd.h>

#include "../../crow.hpp"
#include "stack.hpp"
#include "protobuf_wire_format.h"

#define NONE_LEFT(PTR) (PTR >= _end)
#define BYTES_REMAIN(PTR) (PTR < _end)
#define MAX_NAME_LEN 64
#define MAX_FIELDS   72    // ridiculously wide table

namespace crow {

  class EncoderImpl : public Encoder {
    static const uint8_t UPPER_BIT = (uint8_t)0x80;
  public:
    EncoderImpl(size_t initialCapacity) : Encoder(), _stack(initialCapacity),
          _dataStack(1024), _hdrStack(1024), _fieldMap(), _fields(),
          _structFields(), _haveStructData(false), _structLen(0),
          _structDefFinalized(false), _structBuf(0)  {}

    ~EncoderImpl() { }

int put(const SPFieldDef fieldDef, DynVal value) override {
  if (!fieldDef) {
    assert(false);
    return -1;
  }

  SPFieldInfo field = _findFieldInfo(fieldDef);
  if (!field) {
    field = _newFieldInfo(fieldDef);
  }

  if (value.valid()) {
    writeIndexTag(field);
    _write(field, value);
  } else {
    writeHeaderTag(field);
  }

  return 0;
}

virtual int put(const DynMap &obj) override {
  for (auto it = obj.begin(); it != obj.end(); it++) {
    put (it->first, it->second);
  }
  return 0;
}

/*
    void put(const SPField field, const std::vector<uint8_t>& value) override {
      if (isInvalid(field)) {
        assert(false);
        return;
      }
      writeIndexTag(field);
      writeVarInt(value.size(), staq());
      memcpy(staq().Push(value.size()), value.data(), value.size());
    }
    void put(const SPField field, const uint8_t* bytes, size_t len) override {
      if (isInvalid(field)) {
        assert(false);
        return;
      }
      writeIndexTag(field);
      writeVarInt(len, staq());
      memcpy(staq().Push(len), bytes, len);
    }
    */

    void _flush(int fd, bool headersOnly=false) {
      // flush header
      if (_hdrStack.GetSize() > 0) {
        // copy data
        memcpy(_stack.Push(_hdrStack.GetSize()), _hdrStack.Bottom(), _hdrStack.GetSize());
        _hdrStack.Clear();
      }

      if (headersOnly) { return; }

      // write struct data if defined

      if (_structLen > 0 && _haveStructData) {

        if (_structDefFinalized && !_haveStructData) {
          throw new std::runtime_error("row has no struct data");
        }
        *(_stack.Push(1)) = TROW;
        memcpy(_stack.Push(_structLen), _structBuf.Bottom(), _structLen);
        _structDefFinalized = true;

        // when we have both struct and variable fields, need to write length
        // of variable section after struct data

        if (_fields.size() > _structFields.size()) {
          writeVarInt(_dataStack.GetSize(),_stack);
        }
      }

      // write variable fields

      if (_dataStack.GetSize() > 0) {

        // write TROW, but only if we don't have struct data defined
        if (_structLen == 0) {
          *(_stack.Push(1)) = TROW;
        }

        // copy data
        memcpy(_stack.Push(_dataStack.GetSize()), _dataStack.Bottom(), _dataStack.GetSize());
        _dataStack.Clear();
      }
      _haveStructData = false;

      if (fd > 0) {
        write(fd, (const void *)_stack.Bottom(), _stack.GetSize());
        _stack.Clear();
      }
    }

    virtual void startRow() override {
      _flush(0);
    }

    virtual void startTable(int flags) override {
      _flush(0);
      uint8_t tagid = TTABLE | ((uint8_t)flags & 0x70);
      auto p = _hdrStack.Push(1);
      *p = tagid;
      _fields.clear();
      _structFields.clear();
      _fieldMap.clear();
    }

    virtual void flush(bool headersOnly=false) const override {
      ((EncoderImpl*)this)->_flush(0, headersOnly);
    }
    virtual void flushfd(int fd, bool headersOnly=false) override {
      _flush(fd, headersOnly);
    }
    virtual void endRow(int fd) override {
      if (fd > 0) _flush(fd);
    }


    const uint8_t* data() const override { flush(); return _stack.Bottom(); }

    size_t size() const override { return _stack.GetSize(); }

    void clear() override {
      _stack.Clear();
      _fields.clear();
      _structLen = 0;
      _structFields.clear();
    }

    virtual int struct_hdr(const SPFieldDef fieldDef, int fixedLength = 0) override {

      // check typeId, fixedLength

      if (!fieldDef || !fieldDef->valid()) {
        return -1;
      }
      if (fixedLength == 0 && (fieldDef->typeId == CrowType::TSTRING || fieldDef->typeId == CrowType::TBYTES)) {
        return -2;
      }

      if (_structDefFinalized) {
        throw new std::invalid_argument("Trying to add struct field after first struct row finalized");
      }

      if (_fields.size() > _structFields.size()) {
        throw new std::invalid_argument("All struct columns must come before variable columns");
      }

      // if not provided, determine field size based on type

      fixedLength = (fixedLength > 0 ? fixedLength : byte_size(fieldDef->typeId));

      SPFieldInfo field = _findFieldInfo(fieldDef);
      if (field) {
        return -3; // already defined
      }
      field = _newFieldInfo(fieldDef, fixedLength);

      _structFields.push_back(field);

      _structLen += fixedLength;

      writeIndexTag(field);

      return 0;
    }

    int put_struct(const void *data, size_t struct_size) override {
      if (struct_size == 0 || struct_size != _structLen) {
        char tmp[128];
        snprintf(tmp, sizeof(tmp), "struct size:%lu does not match definition:%lu", struct_size, _structLen);
        throw new std::invalid_argument(tmp);
      }

      if (_structBuf.GetSize() == 0) {
        _structBuf.Push(_structLen);
      }

      //startRow();

      memcpy(_structBuf.Bottom(), data, _structLen);
      _haveStructData = true;

      return 0;
    }

  private:

    /**
     * return _stack or _setStack, depending on _setModeEnabled field.
     */
    Stack & staq() { return _dataStack; } // (_setModeEnabled ? _setStack : _stack); }

    /*
     *
     */
    SPFieldInfo _findFieldInfo(const SPFieldDef fieldDef) {

      auto fit = _fieldMap.find(fieldDef);
      if (fit != _fieldMap.end()) {
        return fit->second;
      }

      return SPFieldInfo();
    }

    SPFieldInfo _newFieldInfo(const SPFieldDef fieldDef, uint32_t fixedSize = 0) {
      SPFieldInfo field;
      field = std::make_shared<FieldInfo>(fieldDef, (uint8_t)_fieldMap.size(), fixedSize);
      _fieldMap[fieldDef] = field;
      return field;
    }

    void _write(const SPFieldInfo field, DynVal value) {

      switch (field->typeId){
        case TINT8: {
          uint8_t* ptr = staq().Push(1);
          *ptr = (uint8_t)value.as_i8();
          break;
        }
        case TUINT8: {
          uint8_t* ptr = staq().Push(1);
          *ptr = value.as_u8();
          break;
        }
        case TINT16:
          writeVarInt(ZigZagEncode32(value.as_i16()), staq());
          break;
        case TUINT16:
          writeVarInt(value.as_u16(), staq());
          break;
        case TINT32:
          writeVarInt(ZigZagEncode32(value.as_i32()), staq());
          break;
        case TUINT32:
          writeVarInt(value.as_u32(), staq());
          break;
        case TINT64:
          writeVarInt(ZigZagEncode64(value.as_i64()), staq());
          break;
        case TUINT64:
          writeVarInt(value.as_u64(), staq());
          break;
        case TFLOAT64:
          writeFixed64(EncodeDouble(value.as_double()), staq());
          break;
        case TFLOAT32:
          writeFixed32(EncodeFloat(value.as_float()), staq());
          break;
        case TSTRING: {
          std::string s = value.as_s();
          writeVarInt(s.length(), staq());
          memcpy(staq().Push(s.length()), s.c_str(), s.length());
          break;
        }
        default:
          assert(false);
          break;
      }
    }

    void writeFixed64(uint64_t value, Stack &stack) {
      union { uint64_t ival; uint8_t bytes[8];};
      ival = value;
      memcpy(stack.Push(sizeof(value)), bytes, sizeof(bytes));
    }
    void writeFixed32(uint32_t value, Stack &stack) {
      union { uint32_t ival; uint8_t bytes[4];};
      ival = value;
      memcpy(stack.Push(sizeof(value)), bytes, sizeof(bytes));
    }

    void writeHeaderTag(const SPFieldInfo field) {
      if (field->isWritten) { return; }

      Stack &stack = _hdrStack;
      size_t namelen = field->name.length();

      uint8_t tagbyte = CrowTag::THFIELD;
      if (field->schemaId > 0) { tagbyte |= FIELDINFO_FLAG_HAS_SUBID; }
      if (namelen > 0) { tagbyte |= FIELDINFO_FLAG_HAS_NAME; }
      if (field->isStructField()) { tagbyte |= FIELDINFO_FLAG_RAW; }

      uint8_t* ptr = stack.Push(2);
      *ptr++ = tagbyte;
      *ptr++ = field->index;

      // typeid
      ptr = stack.Push(1);
      ptr[0] = field->typeId;

      // id and subid (if set)
      writeVarInt(field->id, stack);
      if (field->schemaId > 0) { writeVarInt(field->schemaId, stack); }

      // name (if set)
      if (namelen > 0) {
        writeVarInt(namelen, stack);
        ptr = stack.Push(namelen);
        memcpy(ptr,field->name.c_str(),namelen);
      }

      if (field->isStructField()) {
        writeVarInt(field->structFieldLength, stack);
      }
      // mark as written, so we dont write FIELDINFO more than once
      field->isWritten = true;
      //((Field*)pField)->_written = true;
    }

    /*
     * one byte 0x80 | index
     */
    void writeIndexTag(const SPFieldInfo field) {

      if (field->isWritten) {

        Stack &stack = _dataStack;
        uint8_t* ptr = stack.Push(1);
        ptr[0] = field->index | UPPER_BIT;

      } else {
        writeHeaderTag(field);
        if (!field->isStructField()) {
          // field index on data row
          uint8_t* ptr = _dataStack.Push(1);
          ptr[0] = field->index | UPPER_BIT;
        }
      }
    }

    size_t writeVarInt(uint64_t value, Stack &stack) {
      size_t i=0;
      while (true) {
        i++;
        uint8_t b = (uint8_t)(value & 0x07F);
        value = value >> 7;
        if (0L == value) {
          uint8_t* ptr = stack.Push(1);
          *ptr = b;
          break;
        }
        uint8_t* p = stack.Push(1);
        *p = (b | 0x80u);
      }

      return i;
    }

    Stack _stack, _dataStack, _hdrStack;
    std::map<const SPFieldDef, SPFieldInfo> _fieldMap;
    std::vector<SPFieldInfo> _fields;
    std::vector<SPFieldInfo> _structFields;
    bool   _haveStructData;
    size_t _structLen;
    bool   _structDefFinalized;
    Stack  _structBuf;
  };

  class EncoderFactory {
  public:
    static Encoder* New(size_t initialCapacity = 4096) { return new EncoderImpl(initialCapacity); }
  };

}
#endif // _CROW_ENCODE_IMPL_HPP_

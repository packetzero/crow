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

#define PUT_CONVERT(field, value) { \
  switch(field->typeId) { \
  case TFLOAT64: \
    return put(field, (double)value); \
  case TFLOAT32: \
    return put(field, (float)value); \
  case TINT32: \
    return put(field, (int32_t)value); \
  case TUINT32: \
    return put(field, (uint32_t)value); \
  case TUINT8: \
    return put(field, (uint8_t)value); \
  case TINT8: \
    return put(field, (int8_t)value); \
  case TUINT64: \
    return put(field, (uint64_t)value); \
  case TINT64: \
    return put(field, (int64_t)value); \
  default: \
    return; \
  } \
}

namespace crow {

  class EncoderImpl : public Encoder {
    static const uint8_t UPPER_BIT = (uint8_t)0x80;
  public:
    EncoderImpl(size_t initialCapacity) : Encoder(), _stack(initialCapacity),
          _dataStack(1024), _hdrStack(1024), _fields(),
          _structFields(), _haveStructData(false), _structLen(0),
          _structDefFinalized(false), _structBuf(0), _written()  {}

    ~EncoderImpl() { }

    const SPField fieldFor(CrowType type, uint32_t id, uint32_t subid = 0) override {

      // check for existing

      for (int i=0; i < _fields.size(); i++) {
        const SPField field = _fields[i];
        if (field->id == id && field->subid == subid) return field;
      }
      const SPField f = _newField(type, id, subid);
      return f;
    }

    const SPField fieldFor(CrowType type, std::string name) override {

      // check for existing

      for (int i=0; i < _fields.size(); i++) {
        SPField pField = _fields[i];
        if (pField->name == name) return pField;
      }
      const SPField f = _newField(type, name);
      assert(f && f->typeId != NONE);
      return f;
    }

    // returns SP to field with default initializer (type == NONE)
    const SPField _dummyField() {
      const static SPField spdummy = std::make_shared<Field>();
      return spdummy;
    }

    void put(const SPField field, int32_t value) override {
      if (isInvalid(field)) {
        assert(false);
        return;
      }
      if (field->typeId != TINT32) {
        PUT_CONVERT(field, value);
      }
      writeIndexTag(field);
      writeVarInt(ZigZagEncode32(value), staq());
    }
    void put(const SPField field, uint32_t value) override {
      if (isInvalid(field)) {
        assert(false);
        return;
      }
      if (field->typeId != TUINT32) {
        PUT_CONVERT(field, value);
      }
      writeIndexTag(field);
      writeVarInt(value, staq());
    }
    void put(const SPField field, int64_t value) override {
      if (isInvalid(field)) {
        assert(false);
        return;
      }
      if (field->typeId != TINT64) {
        PUT_CONVERT(field, value);
      }
      writeIndexTag(field);
      writeVarInt(ZigZagEncode64(value), staq());
    }
    void put(const SPField field, uint64_t value) override {
      if (isInvalid(field)) {
        assert(false);
        return;
      }
      if (field->typeId != TUINT64) {
        PUT_CONVERT(field, value);
      }
      writeIndexTag(field);
     writeVarInt(value, staq());
    }
    void put(const SPField field, double value) override {
      if (isInvalid(field)) {
        assert(false);
        return;
      }
      if (field->typeId != TFLOAT64) {
        PUT_CONVERT(field, value);
      }
      writeIndexTag(field);
      writeFixed64(EncodeDouble(value), staq());
    }
    void put(const SPField field, float value) override {
      if (isInvalid(field)) {
        assert(false);
        return;
      }
      if (field->typeId != TFLOAT32) {
        PUT_CONVERT(field, value);
      }
      writeIndexTag(field);
      writeFixed32(EncodeFloat(value), staq());
    }
    void put(const SPField field, const std::string value) override {
      if (isInvalid(field)) {
        assert(false);
        return;
      }
      writeIndexTag(field);
      writeVarInt(value.length(), staq());
      memcpy(staq().Push(value.length()), value.c_str(), value.length());
    }
    void put(const SPField field, const char* str) override {
      if (isInvalid(field)) {
        assert(false);
        return;
      }
      writeIndexTag(field);
      size_t srcLen = strlen(str);
      writeVarInt(srcLen, staq());
      memcpy(staq().Push(srcLen), str, srcLen);
    }
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
    void put(const SPField field, bool value) override {
      put(field, (value ? (uint8_t)1 : (uint8_t)0));
    }

    virtual void put(const SPField field, int8_t value) override {
      if (isInvalid(field)) {
        assert(false);
        return;
      }
      writeIndexTag(field);
      uint8_t* ptr = staq().Push(1);
      *ptr = (uint8_t)value;
    }
    virtual void put(const SPField field, uint8_t value) override {
      if (isInvalid(field)) {
        assert(false);
        return;
      }
      writeIndexTag(field);
      uint8_t* ptr = staq().Push(1);
      *ptr = value;
    }
    virtual void put(const SPField field, int16_t value) override {
      if (isInvalid(field)) {
        assert(false);
        return;
      }
      writeIndexTag(field);
      writeVarInt(ZigZagEncode32(value), staq());
    }
    virtual void put(const SPField field, uint16_t value) override {
      if (isInvalid(field)) {
        assert(false);
        return;
      }
      writeIndexTag(field);
      writeVarInt(value, staq());
    }

    void _flush(int fd = 0) {
      // flush header
      if (_hdrStack.GetSize() > 0) {
        // copy data
        memcpy(_stack.Push(_hdrStack.GetSize()), _hdrStack.Bottom(), _hdrStack.GetSize());
        _hdrStack.Clear();
      }

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
      _flush();
    }

    virtual void startTable(int flags) override {
      _flush();
      uint8_t tagid = TTABLE | ((uint8_t)flags & 0x70);
      auto p = _hdrStack.Push(1);
      *p = tagid;
      _fields.clear();
      _structFields.clear();
    }

    virtual void flush() const override {
      ((EncoderImpl*)this)->_flush();
    }
    virtual void flush(int fd) override {
      _flush(fd);
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

    virtual int struct_hdr(CrowType typeId, uint32_t id, uint32_t subid = 0, std::string name = "", int fixedLength = 0) override {

      // check typeId, fixedLength

      if (typeId == 0 || typeId >= CrowType::NUM_TYPES) {
        return -1;
      }
      if (fixedLength == 0 && (typeId == CrowType::TSTRING || typeId == CrowType::TBYTES)) {
        return -2;
      }

      if (_structDefFinalized) {
        throw new std::invalid_argument("Trying to add struct field after first struct row finalized");
      }

      if (_fields.size() > _structFields.size()) {
        throw new std::invalid_argument("All struct columns must come before variable columns");
      }

      for (int i=0; i < _fields.size(); i++) {
        const SPField field = _fields[i];
        if (field->id == id && field->subid == subid && field->name == name) {
          return -3;
        }
      }

      const SPField field = _newStructField(typeId, name, id, subid, fixedLength);
      _structFields.push_back(field);

      _structLen += (fixedLength > 0 ? fixedLength : byte_size(typeId));

      writeIndexTag(field);

      return 0;
    }

    virtual int struct_hdr(CrowType typ, std::string name, int fixedLength = 0) override {
      return struct_hdr(typ, 0, 0, name, fixedLength);
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

    inline bool isInvalid(const SPField f) {
      return (f->typeId == NONE);
    }

    /**
     * return _stack or _setStack, depending on _setModeEnabled field.
     */
    Stack & staq() { return _dataStack; } // (_setModeEnabled ? _setStack : _stack); }

    const SPField _newStructField(CrowType typeId, std::string name, uint32_t id, uint32_t subid, uint32_t fixedLen) {
      // new one
      uint8_t index = (uint8_t)_fields.size();
      if ((index & UPPER_BIT) != 0) {
        // wow, 127 fields?  return a dummy field .. one that doesn't get written
        return _dummyField();
      }
      _fields.push_back(std::make_shared<Field>());
      SPField p = _fields[index];
      p->name = name;
      p->index = index;
      p->typeId = typeId;
      p->id = id;
      p->subid = subid;
      p->fixedLen = fixedLen;
      p->isRaw = true;
      return p;
    }

    const SPField _newField(CrowType typeId, uint32_t id, uint32_t subid=0) {
      // new one
      uint8_t index = (uint8_t)_fields.size();
      if ((index & UPPER_BIT) != 0) {
        // wow, 127 fields?  return a dummy field .. one that doesn't get written
        assert(false);
        return _dummyField();
      }

      _fields.push_back(std::make_shared<Field>());
      SPField p = _fields[index];
      p->index = index;
      p->typeId = typeId;
      p->id = id;
      p->subid = subid;
      return p;
    }


    const SPField _newField(CrowType typeId, std::string name, uint32_t id=0) {
      // new one
      uint8_t index = (uint8_t)_fields.size();
      if ((index & UPPER_BIT) != 0) {
        // wow, 127 fields?  return a dummy field .. one that doesn't get written
        assert(false);
        return _dummyField();
      }
      _fields.push_back(std::make_shared<Field>());
      SPField p = _fields[index];
      p->index = index;
      p->typeId = typeId;
      p->id = id;
      p->name = name;
      return p;
    }
/*
    const Field &_newStructField(CrowType typeId, std::string name, uint32_t id, uint32_t fixedLen) {
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
      p->id = id;
      p->name = name;
      p->fixedLen = fixedLen;
      p->isRaw = true;
      return *p;
    }
*/

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
    /*
     * one byte 0x80 | index
     */
    void writeIndexTag(const SPField field) {

      auto fit = _written.find(field);
      if (fit != _written.end()) {
      //if (field._written) {

        Stack &stack = _dataStack;
        uint8_t* ptr = stack.Push(1);
        ptr[0] = field->index | UPPER_BIT;

      } else {
        Stack &stack = _hdrStack;
        size_t namelen = field->name.length();

        uint8_t tagbyte = CrowTag::THFIELD;
        if (field->subid > 0) { tagbyte |= FIELDINFO_FLAG_HAS_SUBID; }
        if (namelen > 0) { tagbyte |= FIELDINFO_FLAG_HAS_NAME; }
        if (field->isRaw) { tagbyte |= FIELDINFO_FLAG_RAW; }

        uint8_t* ptr = stack.Push(2);
        *ptr++ = tagbyte;
        *ptr++ = field->index;

        // typeid
        ptr = stack.Push(1);
        ptr[0] = field->typeId;

        // id and subid (if set)
        writeVarInt(field->id, stack);
        if (field->subid > 0) { writeVarInt(field->subid, stack); }

        // name (if set)
        if (namelen > 0) {
          writeVarInt(namelen, stack);
          ptr = stack.Push(namelen);
          memcpy(ptr,field->name.c_str(),namelen);
        }

        if (field->isRaw && field->fixedLen > 0) {
          writeVarInt(field->fixedLen, stack);
        }
        // mark as written, so we dont write FIELDINFO more than once
        _written[field] = true;
        //((Field*)pField)->_written = true;

        if (!field->isRaw) {
          // field index on data row
          ptr = _dataStack.Push(1);
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
    std::vector<SPField> _fields;
    std::vector<SPField> _structFields;
    bool _haveStructData;
    size_t _structLen;
    bool _structDefFinalized;
    Stack _structBuf;
    std::map<const SPField,bool> _written;
  };

  class EncoderFactory {
  public:
    static Encoder* New(size_t initialCapacity = 4096) { return new EncoderImpl(initialCapacity); }
  };

}
#endif // _CROW_ENCODE_IMPL_HPP_

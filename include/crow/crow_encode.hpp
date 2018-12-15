#ifndef _CROW_ENCODE_HPP_
#define _CROW_ENCODE_HPP_

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

namespace crow {

  typedef std::shared_ptr<Field> SPField;

  class Encoder {
  public:
    virtual const SPField fieldFor(CrowType type, uint32_t id, uint32_t subid = 0) = 0;
    virtual const SPField fieldFor(CrowType type, std::string name) = 0;
    inline const SPField fieldFor(CrowType type, uint32_t id, uint32_t subid, std::string name) {
      return (id > 0 ? fieldFor(type, id, subid) : fieldFor(type, name));
    }

    virtual void put(const SPField pField, int32_t value) = 0;
    virtual void put(const SPField pField, uint32_t value) = 0;
    virtual void put(const SPField pField, int64_t value) = 0;
    virtual void put(const SPField pField, uint64_t value) = 0;
    virtual void put(const SPField pField, int8_t value) = 0;
    virtual void put(const SPField pField, uint8_t value) = 0;
    virtual void put(const SPField pField, int16_t value) = 0;
    virtual void put(const SPField pField, uint16_t value) = 0;
    virtual void put(const SPField pField, double value) = 0;
    virtual void put(const SPField pField, float value) = 0;
    virtual void put(const SPField pField, const std::string value) = 0;
    virtual void put(const SPField pField, const char* str) = 0;
    virtual void put(const SPField pField, const std::vector<uint8_t>& value) = 0;
    virtual void put(const SPField pField, const uint8_t* bytes, size_t len) = 0;
    virtual void put(const SPField pField, bool value) = 0;

    virtual int struct_hdr(CrowType typeId, uint32_t id, uint32_t subid = 0, std::string name = "", int fixedLength = 0) = 0;
    virtual int struct_hdr(CrowType typ, std::string name, int fixedLength = 0) = 0;
    /**
     * @brief place struct data
     * @throws invalid_argument if struct_size 0 or does not match definition.
     * @returns 0 on success
     */
    virtual int put_struct(const void *data, size_t struct_size) = 0;

    struct EncField {
    public:
      EncField(Encoder* _pEnc, const SPField f) : field(f), pEnc(_pEnc) {}

      void operator=(bool value) {
        pEnc->put(field, (uint8_t)(value ? 1 : 0));
      }

      void operator=(uint32_t value) {
        pEnc->put(field, value);
      }
      void operator=(int32_t value) {
        pEnc->put(field, value);
      }
      void operator=(uint64_t value) {
        pEnc->put(field, value);
      }
      void operator=(int64_t value) {
        pEnc->put(field, value);
      }
      void operator=(uint16_t value) {
        pEnc->put(field, value);
      }
      void operator=(int16_t value) {
        pEnc->put(field, value);
      }
      void operator=(uint8_t value) {
        pEnc->put(field, value);
      }
      void operator=(int8_t value) {
        pEnc->put(field, value);
      }
      void operator=(double value) {
        pEnc->put(field, value);
      }
      void operator=(float value) {
        pEnc->put(field, value);
      }
      void operator=(const std::string value) {
        pEnc->put(field, value);
      }
      void operator=(const char* value) {
        pEnc->put(field, value);
      }
      void operator=(const std::vector<uint8_t>& value) {
        pEnc->put(field, value);
      }

      const SPField field;
      Encoder* pEnc;

    }; // end struct EncField

    EncField operator[] (const SPField field) {
      assert(field && field->typeId != NONE);
      return EncField(this, field);
    }

    virtual void startRow() = 0;

    virtual void startTable(int flags = 0) = 0;

    /*
     * Call at end of data to flush encoding buffers.
     */
    virtual void flush() const = 0;
    virtual void flush(int fd) = 0;

    /*
     * Call at end of row.  If fd > 0, will flush buffers to file.
     */
    virtual void endRow(int fd = 0) = 0;

    virtual const uint8_t* data() const = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;

    virtual ~Encoder() {}
  };

}

#endif // _CROW_HPP_

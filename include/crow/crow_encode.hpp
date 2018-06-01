#ifndef _CROW_ENCODE_HPP_
#define _CROW_ENCODE_HPP_

#include <stdint.h>
#include <string>
#include <vector>


namespace crow {

  class Encoder {
  public:
    virtual Field *fieldFor(CrowType type, uint32_t id, uint32_t subid = 0) = 0;
    virtual Field *fieldFor(CrowType type, std::string name) = 0;
    inline Field *fieldFor(CrowType type, uint32_t id, uint32_t subid, std::string name) {
      return (id > 0 ? fieldFor(type, id, subid) : fieldFor(type, name));
    }

    virtual void put(const Field *pField, int32_t value) = 0;
    virtual void put(const Field *pField, uint32_t value) = 0;
    virtual void put(const Field *pField, int64_t value) = 0;
    virtual void put(const Field *pField, uint64_t value) = 0;
    virtual void put(const Field *pField, int8_t value) = 0;
    virtual void put(const Field *pField, uint8_t value) = 0;
    virtual void put(const Field *pField, int16_t value) = 0;
    virtual void put(const Field *pField, uint16_t value) = 0;
    virtual void put(const Field *pField, double value) = 0;
    virtual void put(const Field *pField, float value) = 0;
    virtual void put(const Field *pField, const std::string value) = 0;
    virtual void put(const Field *pField, const char* str) = 0;
    virtual void put(const Field *pField, const std::vector<uint8_t>& value) = 0;
    virtual void put(const Field *pField, const uint8_t* bytes, size_t len) = 0;
    virtual void put(const Field *pField, bool value) = 0;

    struct EncField {
    public:
      EncField(Encoder* _pEnc, uint32_t _id, uint32_t _subid = 0) : id(_id), subid(_subid), name(), pEnc(_pEnc) {}
      EncField(Encoder* _pEnc, std::string _name) : id(0), subid(0), name(_name), pEnc(_pEnc) {}

      void operator=(bool value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), (uint8_t)(value ? 1 : 0));
      }

      void operator=(uint32_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(int32_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(uint64_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(int64_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(uint16_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(int16_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(uint8_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(int8_t value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(double value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(float value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(const std::string value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(const char* value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }
      void operator=(const std::vector<uint8_t>& value) {
        pEnc->put(pEnc->fieldFor(crow::type_for(value), id, subid, name), value);
      }

      EncField& operator[](int _subid) {
        subid = _subid;
        return *this;
      }

      EncField& operator[](uint32_t _subid) {
        subid = _subid;
        return *this;
      }

      uint32_t id;
      uint32_t subid;
      std::string name;
      Encoder* pEnc;
    };

    EncField operator[] (uint32_t id) {
      return EncField(this, id);
    }
    EncField operator[] (int id) {
      return EncField(this, (uint32_t)id);
    }
    EncField operator[] (std::string name) {
      return EncField(this, name);
    }
    EncField operator[] (uint64_t id_and_subid) {
      uint32_t id = id_and_subid & 0x0FFFFFFFFL;
      uint32_t subid = (id_and_subid >> 32) & 0x0FFFFFFFFL;
      return EncField(this, id, subid);
    }

    inline void put(int32_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(uint32_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(int64_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(uint64_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(int8_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(uint8_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(int16_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(uint16_t value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(double value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(float value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(const std::string value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }
    inline void put(const char* str, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(str), id, subid), str);
    }
    inline void put(const std::vector<uint8_t>& value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(TBYTES, id, subid), value);
    }
    inline void put(const uint8_t* bytes, size_t len, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(TBYTES, id, subid), bytes, len);
    }
    inline void put(bool value, uint32_t id, uint32_t subid = 0) {
      put(fieldFor(crow::type_for(value), id, subid), value);
    }

    inline void put(int32_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(uint32_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(int64_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(uint64_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(int8_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(uint8_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(int16_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(uint16_t value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(double value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(float value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(const std::string value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }
    inline void put(const char* str, const std::string name) {
      put(fieldFor(crow::type_for(str), name), str);
    }
    inline void put(const std::vector<uint8_t>& value, const std::string name) {
      put(fieldFor(TBYTES, name), value);
    }
    inline void put(const uint8_t* bytes, size_t len, const std::string name) {
      put(fieldFor(TBYTES, name), bytes, len);
    }
    inline void put(bool value, const std::string name) {
      put(fieldFor(crow::type_for(value), name), value);
    }

    virtual void startRow() = 0;
    virtual void flush() const = 0;

    virtual const uint8_t* data() const = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;

    virtual ~Encoder() {}
  };

  Encoder* EncoderNew(size_t initialCapacity = 4096);

}

#endif // _CROW_HPP_

#ifndef _CROW_DECODE_HPP_
#define _CROW_DECODE_HPP_

#include <stdint.h>
#include <string>
#include <vector>
#include <map>

namespace crow {

  const int RV_SKIP_VARIABLE_FIELDS = 2;

  class DecoderListener {
  public:
    virtual void onField(SPCFieldInfo, int8_t value, uint8_t flags) {}
    virtual void onField(SPCFieldInfo, uint8_t value, uint8_t flags) {}
    virtual void onField(SPCFieldInfo, int32_t value, uint8_t flags) {}
    virtual void onField(SPCFieldInfo, uint32_t value, uint8_t flags) {}
    virtual void onField(SPCFieldInfo, int64_t value, uint8_t flags) {}
    virtual void onField(SPCFieldInfo, uint64_t value, uint8_t flags) {}
    virtual void onField(SPCFieldInfo, double value, uint8_t flags) {}
    virtual void onField(SPCFieldInfo, const std::string &value, uint8_t flags) {}
    virtual void onField(SPCFieldInfo, const std::vector<uint8_t> value, uint8_t flags) {}
    virtual void onRowStart() {}
    virtual void onRowEnd() {}
    /**
     * @returns 0 by default.  If RV_SKIP_VARIABLE_FIELDS returned,
     * decoder will skip over variable length fields associated with row.
     */
    virtual int onStruct(const uint8_t *data, size_t datalen, const std::vector<SPCFieldInfo> &structFields) { return 0;}
    virtual void onTableStart(uint8_t flags) {}
  };

  class Decoder {
  public:

    /**
     * @brief Decode a single row
     * @return false on success, true if no data remaining or error.
     */
    virtual bool decodeRow(DecoderListener &listener) = 0;

    /**
     * @brief Decode all data and return number of rows.
     */
    virtual uint32_t decode(DecoderListener &listener, uint64_t setId = 0) = 0;

    virtual ~Decoder() {}

    /**
     * returns 0 if no error, otherwise code from errno.h
     */
    virtual int getErrCode() const = 0;

    /**
     * returns bitwise OR of all TYPE_XX encountered in decoding.
     */
    virtual uint64_t getTypeMask() const = 0;

    /**
     * Indicate that value will be added to the fieldIndex of every decoded field.
     * Used for sets of bi-directional data.
     */
    //virtual void setFieldIndexAdd(uint32_t value) = 0;

    /**
     * After decoding is finished, returns number of bytes after placing sets
     */
    virtual size_t getExpandedSize() = 0;

    virtual std::vector<SPCFieldInfo> getFields() = 0;
  };

} // namespace crow

#endif // _CROW_HPP_

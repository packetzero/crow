#ifndef _CROW_ENCODE_HPP_
#define _CROW_ENCODE_HPP_

#include <stdint.h>
#include <string>
#include <vector>
#include <memory>

namespace crow {

  class Encoder {
  public:

    virtual int put(const SPFieldDef field, DynVal value) = 0;
    virtual int put(const DynMap &obj) = 0;

    virtual int struct_hdr(const SPFieldDef field, int fixedLength = 0) = 0;

    /**
     * @brief place struct data
     * @throws invalid_argument if struct_size 0 or does not match definition.
     * @returns 0 on success
     */
    virtual int put_struct(const void *data, size_t struct_size) = 0;

    virtual void startTable(int flags = 0) = 0;

    virtual void startRow() = 0;

    /*
     * Call at end of row.  If fd > 0, will flush buffers to file.
     */
    virtual void endRow(int fd = 0) = 0;

    /*
     * Call at end of data to flush encoding buffers.
     */
    virtual void flush() const = 0;
    virtual void flush(int fd) = 0;

    virtual const uint8_t* data() const = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;

    virtual ~Encoder() {}
  };

}

#endif // _CROW_HPP_

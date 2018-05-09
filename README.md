## Crow : A C++ library for encoding and decoding typed rows

Think of this as a binary CSV.  This library is for encoding and decoding native C types in a compact protobuf-like format.  It's best suited for serializing typed tabular data, where the columns are not known at compile time.  The encoding format is defined in the [Crow Encoding Spec](https://github.com/packetzero/libcrow_cr/blob/master/doc/CrowEncodingSpec.md).

### Why not use protobuf?
One of the challenges with libprotobuf, is the need to compile definitions.  Part of this
is due to the encoding of WireFormat (varint,length-delimited,etc) rather than FieldType
(uint32, int64,double,string).

### Encoding Example

```
auto pEnc = crow::EncoderNew();
PUT_NAME(pEnc, "Name", "bob");
PUT_NAME(pEnc, "Age", 23);
PUT_NAME(pEnc, "Active", true);
enc.putRowSep();

const uint8_t* output = enc.data();  // enc.size() bytes
```

## Decoding Example

```
auto dl = crow::GenericDecoderListener();

auto pDec = crow::DecoderNew(pEncodedData, encodedDataSize);
pDec->decode(dl);

// do something with the data
DecColValue &fieldValue =  dl->_rows[i][fieldIndex];

```

### Included work

- A simplified version of [rapidjson/internal/stack.h](https://github.com/Tencent/rapidjson/blob/master/include/rapidjson/internal/stack.h) is used as the internal buffer.

- Makes use of [libprotobuf encoding](https://developers.google.com/protocol-buffers/docs/encoding) methodology (Base 128 Varint, ZigZag transform, tags, length-delimited strings)

### Building with tests
You need to provide include and lib paths for gtest in order to build the tests.
```
mkdir build && cd build
MAKE_TESTS=1 cmake -DCMAKE_CXX_FLAGS=-I~/googletest/include -DCMAKE_EXE_LINKER_FLAGS=-L~/googletest/lib ..
make
```

## Crow : A C++11 library for encoding and decoding typed rows

Think of this as a binary CSV.  This library is for encoding and decoding native C types in a compact protobuf-like format.  It's best suited for serializing typed tabular data, where the columns are not known at compile time.  The encoding format is defined in the [Crow Encoding Spec](https://github.com/packetzero/libcrow_cr/blob/master/doc/CrowEncodingSpec.md).  This project includes [dyno](https://github.com/packetzero/dyno) header library as a submodule.
This implementation supports decorators, a set of fields that apply to all rows, but are stored only once to avoid repetition.

### Why not use protobuf?
One of the challenges with libprotobuf, is the need to compile definitions.  Part of this
is due to the encoding of WireFormat (varint,length-delimited,etc) rather than FieldType
(uint32, int64,double,string).

### Encoding Example - Field names

```
auto pEnc = crow::EncoderNew();
auto &enc = *pEnc;

static const SPFieldDef NAME = FieldDef::alloc(TSTRING, "name");
static const SPFieldDef AGE = FieldDef::alloc(TINT32, "age");

enc.put(NAME, "Bob");
enc.put(AGE, 23);
enc.endRow();

const uint8_t* output = enc.data();  // enc.size() bytes
```

### Encoding Example - Fields with IDs

```
static const SPFieldDef IpfixTcpSourcePort = FieldDef::alloc(TUINT16, 182);
static const SPFieldDef IpfixTcpDestPort = FieldDef::alloc(TUINT16, 183);

enc.put(IpfixTcpSourcePort, 12243);
enc.put(IpfixTcpSourcePort, 443);
enc.endRow();

const uint8_t* output = enc.data();  // enc.size() bytes
```

## Decoding Example

```
auto dl = crow::GenericDecoderListener();

auto pDec = crow::DecoderNew(pEncodedData, encodedDataSize);
pDec->decode(dl);

// do something with the data
for (auto row : dl->_rows) {
  for (auto it = row.begin(); it != row.end; it++) {
    crow::SPCFieldInfo field = it->first;
    crow::DecColValue &value = it->second;

    printf("%s : %s\n", field->name.c_str(), value.as_s().c_str());
  }
}

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

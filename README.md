## Simplepb : A C++ library for simplified protobuf-like encoding and decoding

This library is for encoding and decoding native C types in a compact protobuf-like format.  It's best suited for serializing typed tabular data, where the columns are not known at compile time.


One of the challenges with libprotobuf, is the need to compile definitions.  This library does
not require definition files.  The format output here is not compatible with protobuf,
because we use two-byte tags (fieldIndex, fieldType), while protobuf uses a
single byte field tag (fieldNum << 3 | wireformat).  By encoding the wireformat (varint,length-delimited,etc) rather than the fieldType (uint32, int64,double,string), protobuf requires a definition
to reconstitute fields.

### Features

 - No definition files
 - Only support primitive and string types
 - Only to and from memory buffers
 - All fields optional
 - repeated fields are not packed

 ### Encoding example

```
struct A {
  int32_t     i32val;
  uint64_t    u64val;
  std::string strval;
};

A a = { 45, 0x0FFFFFFFFFFFF, "hello"};
auto pEnc = simplepb::EncoderNew();
uint32_t fieldIndex = 0;
pEnc->put(fieldIndex++, a.i32val);
pEnc->put(fieldIndex++, a.i64val);
pEnc->put(fieldIndex++, a.strval);
const std::vector<uint8_t>& result = pEnc->buffer();

```

### Decoding example

```
auto listener = MyDecoderListener();
auto pDec = simplepb::DecoderNew();
pDec->decode(byteVec, listener);
```

where MyDecoderListener is implemented by your application.
Here is an example that just collects strings:

```
class MyDecoderListener : public DecoderListener {
public:
  void onField(uint32_t fieldIndex, std::string value) override {
    strings.push_back(value);
  }
  std::vector<std::string> strings;
};
```

#include <gtest/gtest.h>
#include "../include/crow.hpp"

#ifdef NEVER
/*
 * Some made-up structures to test relational (SETREL) scenarios
 */

struct NetAddr {
  uint32_t    addr;
  std::string hostname;
  std::string ifname;
};

struct ProcessInfo {
  std::string path;
  uint32_t   pid;
  uint32_t   ppid;
};

struct NetFlow {
  uint8_t ipproto;
  uint8_t isV6;
  uint16_t flags;

  NetAddr *laddr;
  NetAddr *raddr;
  uint16_t lport;
  uint16_t rport;

  uint64_t lbytes;
  uint64_t lpackets;
  uint64_t rbytes;
  uint64_t rpackets;

  ProcessInfo *process;
};

enum FlowColumns {
  COL_REVERSE=1,    // if bit0 set, field is reverse direction. More compact encoding when using bit0 rather than bit32 for example

  COL_IPPROTO=2,
  COL_ISV6=4,
  COL_FLAGS=6,

  COL_ADDR_BYTES=8,
  // ADDR_BYTES_LEFT_TO_RIGHT=8
  // ADDR_BYTES_RIGHT_TO_LEFT=9  (COL_ADDR_BYTES | COL_REVERSE)

  COL_ADDR_HOSTNAME=10,
  COL_ADDR_IFNAME=12,

  COL_PORT=14,

  COL_PATH=16,
  COL_PID=18,
  COL_PPID=20,

  COL_STAT_BYTES=22,
  COL_STAT_PACKETS=24

};

#define	IPPROTO_TCP		6
static NetAddr gAddrLocal = { 0x0A0A0A0A, "mylaptop.local", "en0"};
static NetAddr gAddrGithub = { 0xc01efd71, "www.github.com", ""};
static NetAddr gAddrNhl = { 0x6857695d, "www.nhl.com", ""};

static ProcessInfo gProcessGit = { "/usr/bin/git", 5432, 210};
static ProcessInfo gProcessSafari = { "/Applications/Safari.app/Contents/MacOS/Safari", 4321, 3210};

static NetFlow     _flows[] = {
  { IPPROTO_TCP, 0, 0, &gAddrLocal, &gAddrGithub, 23411, 443, 1111, 11, 55555, 55, &gProcessSafari},
  { IPPROTO_TCP, 0, 0, &gAddrLocal, &gAddrGithub, 9155, 22, 2222, 22, 44444, 44, &gProcessGit},
  { IPPROTO_TCP, 0, 0, &gAddrLocal, &gAddrNhl, 19190, 443, 333, 3, 33333, 33, &gProcessSafari},
  { IPPROTO_TCP, 0, 0, &gAddrLocal, &gAddrNhl, 19190, 443, 333, 3, 33333, 33, &gProcessSafari},
  { IPPROTO_TCP, 0, 0, &gAddrLocal, &gAddrNhl, 19190, 443, 333, 3, 33333, 33, &gProcessSafari},
  { IPPROTO_TCP, 0, 0, &gAddrLocal, &gAddrNhl, 19190, 443, 333, 3, 33333, 33, &gProcessSafari},
  { IPPROTO_TCP, 0, 0, &gAddrLocal, &gAddrNhl, 19190, 443, 333, 3, 33333, 33, &gProcessSafari},
};

class SetRefTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
  }
};


TEST_F(SetRefTest, basic) {
  auto pEnc = crow::EncoderNew();
  auto &enc = *pEnc;
  auto pSetEnc = crow::EncoderNew();

  for (auto &flow : _flows) {
    enc.put(COL_IPPROTO, flow.ipproto);
    enc.put(COL_ISV6, flow.isV6);
    enc.put(COL_FLAGS, flow.flags);

    enc.put(COL_PORT, flow.lport);
    enc.put(COL_PORT | COL_REVERSE, flow.rport);

    enc.put(COL_STAT_BYTES , flow.lbytes);
    enc.put(COL_STAT_PACKETS , flow.lpackets);
    enc.put(COL_STAT_BYTES | COL_REVERSE, flow.rbytes);
    enc.put(COL_STAT_PACKETS | COL_REVERSE, flow.rpackets);

    if (enc.putSetRef((uint64_t)flow.laddr)) {
      pSetEnc->clear();
      pSetEnc->put(COL_ADDR_BYTES, flow.laddr->addr);
      pSetEnc->put(COL_ADDR_HOSTNAME, flow.laddr->hostname);
      pSetEnc->put(COL_ADDR_IFNAME, flow.laddr->ifname);
      enc.putSet((uint64_t)flow.laddr, *pSetEnc);
    }

    if (enc.putSetRef((uint64_t)flow.raddr, COL_REVERSE)) {
      pSetEnc->clear();
      pSetEnc->put(COL_ADDR_BYTES, flow.raddr->addr);
      pSetEnc->put(COL_ADDR_HOSTNAME, flow.raddr->hostname);
      pSetEnc->put(COL_ADDR_IFNAME, flow.raddr->ifname);
      enc.putSet((uint64_t)flow.raddr, *pSetEnc, COL_REVERSE);
    }

    enc.put(COL_STAT_BYTES, flow.lbytes);
    enc.put(COL_STAT_PACKETS, flow.lpackets);
    enc.put(COL_STAT_BYTES | COL_REVERSE, flow.rbytes);
    enc.put(COL_STAT_PACKETS | COL_REVERSE, flow.rpackets);

    if (enc.putSetRef((uint64_t)flow.process)) {
      pSetEnc->clear();
      pSetEnc->put(COL_PATH, flow.process->path);
      pSetEnc->put(COL_PID, flow.process->pid);
      pSetEnc->put(COL_PPID, flow.process->ppid);
      enc.putSet((uint64_t)flow.process, *pSetEnc);
    }

    enc.putRowSep();
  }

  auto pDec = crow::DecoderNew(enc.data(), enc.size());
  auto dl = crow::GenericDecoderListener();
  pDec->decode(dl);

  //printf("encoded size:%lu expanded:%lu\n", enc.size(), pDec->getExpandedSize());

  // column indexes here are based on order of encoder put calls above, not fieldIndex values
  ASSERT_EQ("6", dl._rows[0][0].strval);
  ASSERT_EQ(443, dl._rows[0][4].i32val);
  ASSERT_EQ("www.github.com", dl._rows[0][13].strval);
  ASSERT_EQ(COL_ADDR_HOSTNAME | COL_REVERSE, dl._rows[0][13].fieldIndex);
  ASSERT_EQ("mylaptop.local", dl._rows[0][10].strval);
  ASSERT_EQ(COL_ADDR_HOSTNAME , dl._rows[0][10].fieldIndex);

  ASSERT_EQ("6", dl._rows[1][0].strval);
  ASSERT_EQ(22, dl._rows[1][4].i32val);
  ASSERT_EQ("www.github.com", dl._rows[1][13].strval);
  ASSERT_EQ(COL_ADDR_HOSTNAME | COL_REVERSE, dl._rows[1][13].fieldIndex);
  ASSERT_EQ("mylaptop.local", dl._rows[1][10].strval);
  ASSERT_EQ(COL_ADDR_HOSTNAME , dl._rows[1][10].fieldIndex);

  ASSERT_EQ("6", dl._rows[2][0].strval);
  ASSERT_EQ(443, dl._rows[2][4].i32val);
  ASSERT_EQ("www.nhl.com", dl._rows[2][13].strval);
  ASSERT_EQ(COL_ADDR_HOSTNAME | COL_REVERSE, dl._rows[2][13].fieldIndex);
  ASSERT_EQ("mylaptop.local", dl._rows[2][10].strval);
  ASSERT_EQ(COL_ADDR_HOSTNAME , dl._rows[2][10].fieldIndex);
}
#endif // NEVER

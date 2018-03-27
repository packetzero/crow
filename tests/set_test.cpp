#include <gtest/gtest.h>
#include "../include/crow.hpp"

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
  COL_IPPROTO=1,
  COL_ISV6,
  COL_FLAGS,

  COL_ADDR_BYTES,
  COL_ADDR_HOSTNAME,
  COL_ADDR_IFNAME,

  COL_PORT,

  COL_PATH,
  COL_PID,
  COL_PPID,

  COL_STAT_BYTES,
  COL_STAT_PACKETS

};

#define DIR_LEFT_TO_RIGHT (0)
#define DIR_RIGHT_TO_LEFT (1U << 20)

#define	IPPROTO_TCP		6

static NetAddr     _addrs[] = {
  { 0x0A0A0A0A, "mylaptop.local", "en0"},
  { 0xc01efd71, "www.github.com", ""}
};
static ProcessInfo _processes[] = {
  { "/usr/bin/git", 33233, 2340},
  { "/Applications/Safari.app/Contents/MacOS/Safari", 5555, 4444}
};
static NetFlow     _flows[] = {
  { IPPROTO_TCP, 0, 0, &_addrs[0], &_addrs[1], 23411, 443, 1111, 11, 55555, 55, &_processes[1]},
  { IPPROTO_TCP, 0, 0, &_addrs[0], &_addrs[1], 9155, 22, 2222, 22, 44444, 44, &_processes[0]}
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

    enc.put(COL_PORT | DIR_LEFT_TO_RIGHT, flow.lport);
    enc.put(COL_PORT | DIR_RIGHT_TO_LEFT, flow.rport);

    enc.put(COL_STAT_BYTES | DIR_LEFT_TO_RIGHT, flow.lbytes);
    enc.put(COL_STAT_PACKETS | DIR_LEFT_TO_RIGHT, flow.lpackets);
    enc.put(COL_STAT_BYTES | DIR_RIGHT_TO_LEFT, flow.rbytes);
    enc.put(COL_STAT_PACKETS | DIR_RIGHT_TO_LEFT, flow.rpackets);

    if (enc.putSetRef((uint64_t)flow.laddr, DIR_LEFT_TO_RIGHT)) {
      pSetEnc->clear();
      pSetEnc->put(COL_ADDR_BYTES, flow.laddr->addr);
      pSetEnc->put(COL_ADDR_HOSTNAME, flow.laddr->hostname);
      pSetEnc->put(COL_ADDR_IFNAME, flow.laddr->ifname);
      enc.putSet((uint64_t)flow.laddr, *pSetEnc, DIR_LEFT_TO_RIGHT);
    }

    if (enc.putSetRef((uint64_t)flow.raddr, DIR_RIGHT_TO_LEFT)) {
      pSetEnc->clear();
      pSetEnc->put(COL_ADDR_BYTES, flow.raddr->addr);
      pSetEnc->put(COL_ADDR_HOSTNAME, flow.raddr->hostname);
      pSetEnc->put(COL_ADDR_IFNAME, flow.raddr->ifname);
      enc.putSet((uint64_t)flow.raddr, *pSetEnc, DIR_RIGHT_TO_LEFT);
    }

    if (enc.putSetRef((uint64_t)flow.process)) {
      pSetEnc->clear();
      pSetEnc->put(COL_PATH, flow.process->path);
      pSetEnc->put(COL_PID, flow.process->pid);
      pSetEnc->put(COL_PPID, flow.process->ppid);
      enc.putSet((uint64_t)flow.process, *pSetEnc);
    }

    enc.putRowSep();
  }

  const uint8_t* result = enc.data();
  
  auto pDec = crow::DecoderNew(enc.data(), enc.size());
  auto dl = crow::DecoderLogger();
  pDec->decode(dl);
  
  const std::string res = dl.str();
  printf("encoded size:%lu\n", enc.size());

  //ASSERT_EQ("[ 0] int32 47,[ 1] uint64 65535,[ 2] \"hello\",[ 3] double 123.456,", dl.str());

  
/*
  ASSERT_EQ(3, enc.size());
  ASSERT_EQ(0, result[0]);
  ASSERT_EQ(2, result[1]);
  ASSERT_EQ(7, result[2]);

  enc.clear();

  val = 0x8000FFFFU;
  enc.put(1, val);
  result=enc.data();
  ASSERT_EQ(7, enc.size());
  ASSERT_EQ(1, result[0]);
  ASSERT_EQ(2, result[1]);

  delete pEnc;
*/
}

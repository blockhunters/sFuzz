#pragma once
#include <iostream>
#include <vector>
#include <libdevcore/FixedHash.h>
#include <fstream>

#define unlikely(_x)  __builtin_expect(!!(_x), 0)
#define likely(_x)   __builtin_expect(!!(_x), 1)

#define bV "━"
#define bV2 bV bV
#define bV5 bV2 bV2
#define bV10 bV5 bV5
#define bV20 bV10 bV10
#define bV30 bV20 bV10
#define bV50 bV30 bV20

#define bH "┃"
#define bBL "┗"
#define bBR "┛"
#define bTL "┏"
#define bTR "┓"
#define bLTR "┣"
#define bRTR "┫"
#define bBTR "┻"
#define bTTR "┳"
#define bCR "╋"

#define cGRN "\x1b[0;32m"
#define cRST "\x1b[0m"
#define Bold "\e[1m"
#define CURSOR_HIDE "\x1b[?25l"
#define FF(_b)  (0xff << ((_b) << 3))


using namespace std;
using namespace dev;

namespace fuzzer {
  typedef uint8_t u8;
  typedef uint16_t u16;
  typedef uint32_t u32;
  typedef uint64_t u64;
  typedef int8_t   s8;
  typedef int16_t  s16;
  typedef int32_t  s32;
  typedef int64_t  s64;
  
  static u32 SPLICE_CYCLES = 15;
  static u32 MAX_DET_EXTRAS = 200;
  static u32 HAVOC_BLK_XL = 640;
  static u32 HAVOC_BLK_SMALL = 32;
  static u32 HAVOC_BLK_MEDIUM = 128;
  static u32 HAVOC_BLK_LARGE = 320;
  static u32 MAX_FILE = (1024 * 100); // 100kb
  static u32 MIN_AUTO_EXTRA = 3;
  static u32 MAX_AUTO_EXTRA = 33;
  static u32 MAX_AUTO_EXTRAS = 200;
  static int STAGE_FLIP1 = 0;
  static int STAGE_FLIP2 = 1;
  static int STAGE_FLIP4 = 2;
  static int STAGE_FLIP8 = 3;
  static int STAGE_FLIP16 = 4;
  static int STAGE_FLIP32 = 5;
  static int STAGE_ARITH8 = 6;
  static int STAGE_ARITH16 = 7;
  static int STAGE_ARITH32 = 8;
  static int STAGE_INTEREST8 = 9;
  static int STAGE_INTEREST16 = 10;
  static int STAGE_INTEREST32 = 11;
  static int STAGE_EXTRAS_UO = 12;
  static int STAGE_EXTRAS_UI = 13;
  static int STAGE_HAVOC = 14;
  static int HAVOC_STACK_POW2 = 7;
  static int HAVOC_CYCLES_INIT = 1024;
  static int HAVOC_CYCLES = 256;
  static int SPLICE_HAVOC = 32;
  static int HAVOC_MIN = 16;
  static int EFF_MIN_LEN = 1;
  static int EFF_MAP_SCALE2 = 4; // 32 bytes block
  static int MAP_SIZE_POW2 = 16;
  static int MAP_SIZE = (1 << MAP_SIZE_POW2);
  static int HASH_CONST = 0xa5b35705;
  static int ARITH_MAX = 35;
  static int EFF_MAX_PERC = 90;
  static s8 INTERESTING_8[] = { -128, -1, 0, 1, 16, 32, 64, 100, 127};
  static s16 INTERESTING_16[] = {-128, -1, 0, 1, 16, 32, 64, 100, 127, -32768, -129, 128, 255, 256, 512, 1000, 1024, 4096, 32767};
  static s32 INTERESTING_32[] = {-128, -1, 0, 1, 16, 32, 64, 100, 127, -32768, -129, 128, 255, 256, 512, 1000, 1024, 4096, 32767, -2147483648, -100663046, -32769, 32768, 65535, 65536, 100663045, 2147483647};
  
  /* Scale position: 1 efficient block contains 8 bytes */
  int effAPos(int p);
  /* Divide with remainder */
  int effRem(int x);
  /* Count number of efficient block*/
  int effALen(int l);
  /* Len in effector map from position p to p + l */
  int effSpanALen(int p, int l);
  bool couldBeBitflip(u32 xorVal);
  bool couldBeArith(u32 oldVal, u32 newVal, u8 len);
  bool couldBeInterest(u32 oldVal, u32 newVal, u8 blen, u8 checkLe);
  u32 chooseBlockLen(u32 limit);
  u32 UR(u32 limit);
  /* Swap 2 bytes */
  u16 swap16(u16 x);
  /* Swap 4 bytes */
  u32 swap32(u32 x);
  /* Locate differents */
  void locateDiffs(byte* ptr1, byte* ptr2, u32 len, s32* first, s32* last);
  /* count bits */
  u32 coutBits(u8 *mem);
  string formatDuration(int duration);
  string padStr(string str, int len);
  u32 countBytes(u8* mem);
  /* Data struct */
  struct ExtraData {
    bytes data;
    u32 hitCount;
  };
}

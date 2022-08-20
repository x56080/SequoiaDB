#include "ossTypes.h"
#include "randomBsonGenerator.h"
#include "vessel/keyString.h"
#include "vessel/keyStringBuilder.h"
#include "vessel/orderingWrapper.h"
#include "vessel/prefixedKeyString.h"
#include "vessel/prefixGenerator.h"
#include "vessel/btreeNode.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace engine
{
namespace vessel
{
   INT32 compareString(const std::string &l_prefix,
                       const std::string &l_suffix,
                       const std::string &r_prefix,
                       const std::string &r_suffix)
   {
      prefixedKeyString l(slice(l_suffix.size(), l_suffix.data()),
                          slice(l_prefix.size(), l_prefix.data()));
      prefixedKeyString r(slice(r_suffix.size(), r_suffix.data()),
                          slice(r_prefix.size(), r_prefix.data()));
      return l.compare(r);
   }

   void oidCompressionTest(const UINT32 capacity)
   {
      prefixGenerator pg;
      pg.setOptions({8, 0, 4, 1});
      OID oids[capacity];
      ossPoolVector<slice> sliceV;
      ossPoolVector<prefixGenerator::prefixItem> out;
      for (UINT32 i = 0; i < capacity; i++)
      {
         oids[i].init();
         sliceV.emplace_back(sizeof(OID), oids[i].getData());
      }

      //prefixGenerator::resultStat r = 
      pg.generate(sliceV, out);
      // std::cout << r.totalSavedSize << " " << r.compressionRatio <<
      // std::endl;
   }

   void randomKeyStringTest(UINT32 nkeys, UINT32 objNums)
   {
      randomBsonGenerator bg;
      prefixGenerator pg;
      pg.setOptions({0, 0, 4, 1});
      keyStringBuilder<> ksb;
      std::vector<keyString> v;
      ossPoolVector<slice> sliceV;
      for (UINT32 i = 0; i < objNums; i++)
      {
         ksb.reset();
         BSONObj obj = bg.randomBson(nkeys);
         orderingWrapper ord(0, nkeys);
         recordID rid(nkeys, i);
         ksb.buildIndexEntryKey(obj, ord, rid);
         keyString ks = ksb.getShallowKeyString();
         v.push_back(ks);
         v.back().getOwned();
      }
      std::sort(v.begin(),
                v.end(),
                [&](const keyString &key1, const keyString &key2) -> INT32 {
                   if (key1.compare(key2) < 0)
                   {
                      return TRUE;
                   }
                   else
                   {
                      return FALSE;
                   }
                });
      UINT32 totalSize = 0;
      for (UINT32 i = 0; i < objNums; i++)
      {
         sliceV.push_back(v[i].getDataSlice());
         totalSize += v[i].getRawDataSize();
      }
      ossPoolVector<prefixGenerator::prefixItem> out;
      prefixGenerator::resultStat r = pg.generate(sliceV, out);
      std::cout << r.totalSavedSize << " " << r.compressionRatio << std::endl;
   }

   TEST(prefix_compression_test, base_string)
   {
      randomBsonGenerator bg;
      constexpr UINT32 size = 30;
      UINT32 nums = 3000;
      vector<unique_ptr<CHAR[]>> strV(nums);
      ossPoolVector<slice> sliceV;
      for (UINT32 i = 0; i < nums; i++)
      {
         strV[i].reset(new CHAR[size]);
         bg.randomToAssign(strV[i].get(), size);
         sliceV.emplace_back(size, strV[i].get());
      }
      std::sort(sliceV.begin(),
                sliceV.end(),
                [&](const slice &key1, const slice &key2) -> INT32 {
                   if (key1.compare(key2) < 0)
                   {
                      return TRUE;
                   }
                   else
                   {
                      return FALSE;
                   }
                });

      prefixGenerator pg;
      pg.setOptions({8, 0, 4, 1});
      ossPoolVector<prefixGenerator::prefixItem> out;
      auto r = pg.generate(sliceV, out);
      std::cout << r.totalSavedSize << " " << r.compressionRatio << std::endl;
   }

   TEST(prefix_compression_test, base_oid)
   {
      oidCompressionTest(10);
      oidCompressionTest(100);
      oidCompressionTest(1000);
      oidCompressionTest(3000);
   }

   TEST(prefix_compression_test, base_keystring)
   {
      randomKeyStringTest(3, 10);
      randomKeyStringTest(3, 100);
      randomKeyStringTest(3, 1000);
      randomKeyStringTest(3, 3000);
   }

   BOOLEAN equalPrefixItem(const prefixGenerator::prefixItem &l,
                           const CHAR *ptr,
                           UINT32 size,
                           INT32 low,
                           INT32 high)
   {
      if (l.low != low || l.high != high || l.prefix.size() != size ||
          0 != ossMemcmp(l.prefix.data(), ptr, size))
      {
         return FALSE;
      }
      else
      {
         return TRUE;
      }
   }

   TEST(prefix_compression_test, base_prefix_generate)
   {
      prefixGenerator pg;
      pg.setOptions({0, 0, 4, 0});
      ossPoolVector<prefixGenerator::prefixItem> out;
      ossPoolVector<ossPoolString> v;
      ossPoolVector<slice> sliceV;
      CHAR buf[100] = {};
      UINT32 pos = 0;
      for (auto x : {"a", "a", "a", "abcd", "abce", "abde", "acec", "acfg"})
      {
         UINT32 size = strlen(x);
         memcpy(buf + pos, x, size);
         sliceV.emplace_back(size, buf + pos);
         pos += size;
      }
      auto r = pg.generate(sliceV, out);
      UINT32 outPos = 0;
      EXPECT_EQ(8, r.totalSavedSize);
      EXPECT_EQ(equalPrefixItem(out[outPos++], "a", 1, 0, 3), TRUE);
      EXPECT_EQ(equalPrefixItem(out[outPos++], "ab", 2, 3, 6), TRUE);
      EXPECT_EQ(equalPrefixItem(out[outPos++], "ac", 2, 6, 8), TRUE);

      v.clear();
      sliceV.clear();
      pos = 0;
      for (auto x : {"abcd", "abce", "abde", "acec", "acfg"})
      {
         UINT32 size = strlen(x);
         memcpy(buf + pos, x, size);
         sliceV.emplace_back(size, buf + pos);
         pos += size;
      }
      r = pg.generate(sliceV, out);
      outPos = 0;
      EXPECT_EQ(6, r.totalSavedSize);
      EXPECT_EQ(equalPrefixItem(out[outPos++], "ab", 2, 0, 3), TRUE);
      EXPECT_EQ(equalPrefixItem(out[outPos++], "ac", 2, 3, 5), TRUE);

      v.clear();
      sliceV.clear();
      pos = 0;
      for (auto x :
           {"abcd", "abce", "abde", "bcde", "bcdf", "bcdf", "cd", "dd"})
      {
         UINT32 size = strlen(x);
         memcpy(buf + pos, x, size);
         sliceV.emplace_back(size, buf + pos);
         pos += size;
      }
      r = pg.generate(sliceV, out);
      outPos = 0;
      EXPECT_EQ(10, r.totalSavedSize);
      EXPECT_EQ(equalPrefixItem(out[outPos++], "ab", 2, 0, 3), TRUE);
      EXPECT_EQ(equalPrefixItem(out[outPos++], "bcd", 3, 3, 6), TRUE);
      EXPECT_EQ(equalPrefixItem(out[outPos++], "", 0, 6, 8), TRUE);

      v.clear();
      sliceV.clear();
      pos = 0;
      for (auto x : {"abc", "bc", "ce", "cef", "cefg"})
      {
         UINT32 size = strlen(x);
         memcpy(buf + pos, x, size);
         sliceV.emplace_back(size, buf + pos);
         pos += size;
      }
      r = pg.generate(sliceV, out);
      outPos = 0;
      EXPECT_EQ(4, r.totalSavedSize);
      EXPECT_EQ(equalPrefixItem(out[outPos++], "", 0, 0, 2), TRUE);
      EXPECT_EQ(equalPrefixItem(out[outPos++], "ce", 2, 2, 5), TRUE);
   }
} // namespace vessel
} // namespace engine
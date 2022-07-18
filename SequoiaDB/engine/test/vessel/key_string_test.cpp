#include "../bson/bsonDecimal.h"
#include "common_decimal_fun.h"
#include <bitset>
#include <cstdint>
#include <gtest/gtest.h>
#include <iostream>
#include <limits>
#include <string>
#include "vessel/keyStringBuilder.h"
#include "../bson/bsonobjbuilder.h"

namespace engine
{
namespace vessel
{
   std::string double_bits(double num)
   {
      UINT64 encoded;
      memcpy(&encoded, &num, sizeof(encoded));
      return std::bitset<64>(encoded).to_string();
   }
   TEST(key_string_test, base_number)
   {
      INT32 rc = SDB_OK;
      keyStringBuilder<> ksb;
      bson::BSONObjBuilder obj;
      obj.appendDecimal("decimal", "0.3234231231287319719");
      obj.appendNumber("double", 0.3234231231287319719);
      obj.appendNumber("int", 66051);
      obj.appendNumber("long", static_cast<INT64>(66051));
      orderingWrapper ord(0, 4);
      rc = ksb.appendAllElements(obj.obj(), ord);
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getKeyString();
      EXPECT_EQ(42, ks.getSize());
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      ks = ksb.getKeyString();
      EXPECT_EQ(53, ks.getSize());
   }

   TEST(key_string_test, base_ahead_key)
   {
      INT32 rc = SDB_OK;
      keyStringBuilder<> ksb;
      bson::BSONObjBuilder obj;
      orderingWrapper ord(0, 2);
      rc = ksb.appendSignedWithoutType(-255);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.appendSignedWithoutType(-1);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.appendSignedWithoutType(0);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.appendSignedWithoutType(1);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.appendSignedWithoutType(255);
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getKeyString();
      const CHAR *buf = ks.getData();
      for (UINT32 i = 0; i < 4; ++i)
      {
         ASSERT_LT(memcmp(buf + i * 4, buf + (i + 1) * 4, 4), 0);
      }
   }

   TEST(key_string_test, base_cmp_decimal)
   {
      INT32 rc = SDB_OK;
      keyStringBuilder<> ksb;
      bson::BSONObjBuilder obj;
      obj.appendDecimal("decimal", "172657.000003");
      obj.appendDecimal("decimal", "0.000003");
      orderingWrapper ord(0, 32);
      rc = ksb.appendAllElements(obj.obj(), ord);
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getKeyString();
   }
} // namespace vessel
} // namespace engine
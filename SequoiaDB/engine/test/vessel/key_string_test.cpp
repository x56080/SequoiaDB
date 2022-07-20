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
   std::string double_bits(FLOAT64 num)
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
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
      EXPECT_EQ(53, ks.getDataSlice().getSize());
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
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
      const CHAR *buf = ks.getDataSlice().getData();
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
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
   }


   TEST(key_string_test, base_object)
   {

   }

   TEST(key_string_test, base_string)
   {
      INT32 rc = SDB_OK;
      keyStringBuilder<> ksb1;
      keyStringBuilder<> ksb2;
      bson::BSONObj obj1 = BSON("a" << "foobar");
      bson::BSONObj obj2 = BSON("a" << "foobar1");
      orderingWrapper o(1, 1);


      rc = ksb1.appendAllElements(obj1, o);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb1.done();
      ASSERT_EQ(SDB_OK, rc);

      rc = ksb2.appendAllElements(obj2, o);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb2.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks1 = ksb1.getShallowKeyString();
      keyString ks2 = ksb2.getShallowKeyString();

      INT32 res = ks1.compare(ks2);
      ASSERT_LT(res, 0);
   }

   TEST(key_string_test, base_symbol)
   {

   }


   TEST(key_string_test, base_bool)
   {
      INT32 rc = SDB_OK;
      keyStringBuilder<> ksb;
      bson::BSONObjBuilder ob;
      ob.appendBool("foo1", 1);
      ob.appendBool("foo2", 0);
      orderingWrapper o(0, 2);
      rc = ksb.appendAllElements(ob.done(), o);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
      

   }

   TEST(key_string_test, base_date)
   {
      INT32 rc = SDB_OK;
      keyStringBuilder<> ksb1;
      keyStringBuilder<> ksb2;

      bson::Date_t d1(1658216048);
      bson::Date_t d2(1658216050);
      orderingWrapper o(0, 1);

      bson::BSONObj obj1 = BSON("a" << d1);
      bson::BSONObj obj2 = BSON("a" << d2);


      rc = ksb1.appendAllElements(obj1, o);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb1.done();
      ASSERT_EQ(SDB_OK, rc);

      rc = ksb2.appendAllElements(obj2, o);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb2.done();
      ASSERT_EQ(SDB_OK, rc);


      keyString ks1 = ksb1.getShallowKeyString();
      keyString ks2 = ksb1.getShallowKeyString();

   }

   TEST(key_string_test, base_timestamp)
   {
      INT32 rc = SDB_OK;
      keyStringBuilder<> ksb;
      bson::BSONObjBuilder ob;
      bson::OpTime time(1658216048, 0);
      ob.appendTimestamp("foo", time.asDate());
      orderingWrapper o(0, 1);
      rc = ksb.appendAllElements(ob.done(), o);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
   }

   TEST(key_string_test, base_bindata)
   {
      INT32 rc = SDB_OK;
      keyStringBuilder<> ksb;
      bson::BSONObjBuilder ob;
      const CHAR *byteArray = "foobar";
      ob.appendBinData("bin", sizeof(byteArray), 
                       bson::BinDataType::BinDataGeneral,
                       byteArray);
      orderingWrapper o(0, 1);
      rc = ksb.appendAllElements(ob.done(), o);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
   }

   TEST(key_string_test, base_code)
   {



   }

   TEST(key_string_test, base_keyslice)
   {
      INT32 rc = SDB_OK;
      bson::BSONObj obj = BSON("a" << "foobar");
      UINT32 keyBefore = 123;
      orderingWrapper o(0, 1);

      keyStringBuilder<> ksb1;
      rc = ksb1.appendUnsignedWithoutType(keyBefore);
      ASSERT_EQ(SDB_OK, rc);
      ksb1.appendAllElements(obj, o);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb1.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks1 = ksb1.getShallowKeyString();
      
      keyStringBuilder<> ksb2;
      ksb2.appendAllElements(obj, o);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb2.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks2 = ksb2.getShallowKeyString();

      INT32 res = ossMemcmp(ks1.getKeySlice().getData(),
                            ks2.getKeySlice().getData(),
                            ks1.getKeySlice().getSize());
      EXPECT_EQ(res, 0);

      ksb1.reset();
      ksb2.reset();
      std::string str('x', 200);
      bson::BSONObjBuilder ob;
      ob << "name" << str << "foo" << str << "int" << 123;
      obj = ob.obj();
      orderingWrapper ord(0, 3);
      rc = ksb1.appendUnsignedWithoutType(keyBefore);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb1.appendAllElements(obj, ord);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb1.done();
      ASSERT_EQ(SDB_OK, rc);
      ks1 = ksb1.getShallowKeyString();
      
      rc = ksb2.appendAllElements(obj, ord);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb2.done();
      ASSERT_EQ(SDB_OK, rc);
      ks2 = ksb2.getShallowKeyString();

      res = ossMemcmp(ks1.getKeySlice().getData(),
                      ks2.getKeySlice().getData(),
                      ks1.getKeySlice().getSize());
      ASSERT_EQ(res, 0);

   }




} // namespace vessel
} // namespace engin
/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = key_string_test.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/11/2022  ZHY Initial Draft
   Last Changed =

******************************************************************************/

#include "../bson/bsonDecimal.h"
#include "common_decimal_fun.h"
#include <bitset>
#include <cstdint>
#include <gtest/gtest.h>
#include <iostream>
#include <limits>
#include <string>
#include "vessel/globalIndexID.h"
#include "vessel/keyString.h"
#include "vessel/keyStringBuilder.h"
#include "../bson/bsonobjbuilder.h"
#include "vessel/recordID.h"

namespace engine
{
namespace vessel
{
   class key_string_test : public testing::Test
   {
   public:
      static void SetUpTestCase()
      {
      }

      static void TearDownTestCase()
      {
      }

      virtual void SetUp()
      {
         bsb.reset();
         ksb.reset();
      }
      bson::BSONObjBuilder bsb;
      keyStringBuilder<> ksb;
   };

   TEST_F(key_string_test, base_minkey)
   {
      INT32 rc = SDB_OK;
      orderingWrapper ord(0b10, 2);
      bsb.appendMinKey("a");
      bsb.appendMinKey("b");
      rc = ksb.appendAllElements(bsb.done(), ord);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
      BSONObj objFromKey = ks.toBSON(BSON("a" << 1 << "b" << -1), TRUE);
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
   }

   TEST_F(key_string_test, base_undefined)
   {
      INT32 rc = SDB_OK;
      orderingWrapper ord(0b10, 2);
      bsb.appendUndefined("a");
      bsb.appendUndefined("b");
      rc = ksb.appendAllElements(bsb.done(), ord);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
      BSONObj objFromKey = ks.toBSON(BSON("a" << 1 << "b" << -1), TRUE);
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
      EXPECT_EQ(ks.getKeySlice().data()[0],
                static_cast<UINT8>(EncodedType::undefined));
      EXPECT_EQ(ks.getKeySlice().data()[1],
                ~static_cast<UINT8>(EncodedType::undefined));
   }

   TEST_F(key_string_test, base_eoo_jstnull)
   {
      INT32 rc = SDB_OK;
      orderingWrapper ord(0b10, 2);
      bsb.appendNull("a");
      bsb.appendNull("b");
      rc = ksb.appendAllElements(bsb.done(), ord);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
      BSONObj objFromKey = ks.toBSON(BSON("a" << 1 << "b" << -1), TRUE);
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
      EXPECT_EQ(ks.getKeySlice().data()[0],
                static_cast<UINT8>(EncodedType::nullish));
      EXPECT_EQ(ks.getKeySlice().data()[1],
                ~static_cast<UINT8>(EncodedType::nullish));
   }

   TEST_F(key_string_test, base_int)
   {
      INT32 rc = SDB_OK;
      bson::BSONObjBuilder bsb;
      bsb.appendNumber("a", 1);
      bsb.appendNumber("b", -1);
      bson::BSONObj pattern = bsb.obj();
      bsb.reset();
      vector<vector<INT32>> numbers = {{std::numeric_limits<INT32>::min(),
                                        std::numeric_limits<INT32>::min()},
                                       {-255, -255},
                                       {-1, -1},
                                       {0, 0},
                                       {1, 1},
                                       {255, 255},
                                       {std::numeric_limits<INT32>::max(),
                                        std::numeric_limits<INT32>::max()}};
      for (UINT32 i = 0; i < numbers.size(); i++)
      {
         bsb.appendNumber("a", numbers[i][0]);
         bsb.appendNumber("b", numbers[i][1]);
         orderingWrapper ord(0b10, 2);
         bson::BSONObj obj = bsb.obj();
         rc = ksb.appendAllElements(obj, ord);
         ASSERT_EQ(SDB_OK, rc);
         rc = ksb.done();
         ASSERT_EQ(SDB_OK, rc);
         keyString ks = ksb.getShallowKeyString();
         rc = ks.getOwned();
         ASSERT_EQ(SDB_OK, rc);
         bson::BSONObj objFromKey = ks.toBSON(pattern, TRUE);
         EXPECT_EQ(obj.woCompare(objFromKey), 0);
         bsb.reset();
         ksb.reset();
      }
   }

   TEST_F(key_string_test, base_long)
   {
      INT32 rc = SDB_OK;
      bson::BSONObjBuilder bsb;
      bsb.appendNumber("a", 1);
      bsb.appendNumber("b", -1);
      bson::BSONObj pattern = bsb.obj();
      bsb.reset();
      vector<vector<INT64>> numbers = {{std::numeric_limits<INT64>::min(),
                                        std::numeric_limits<INT64>::min()},
                                       {-255, -255},
                                       {-1, -1},
                                       {0, 0},
                                       {1, 1},
                                       {255, 255},
                                       {std::numeric_limits<INT64>::max(),
                                        std::numeric_limits<INT64>::max()}};
      for (UINT32 i = 0; i < numbers.size(); i++)
      {
         bsb.appendNumber("a", static_cast<INT64>(numbers[i][0]));
         bsb.appendNumber("b", static_cast<INT64>(numbers[i][1]));
         orderingWrapper ord(0b10, 2);
         bson::BSONObj obj = bsb.obj();
         rc = ksb.appendAllElements(obj, ord);
         ASSERT_EQ(SDB_OK, rc);
         rc = ksb.done();
         ASSERT_EQ(SDB_OK, rc);
         keyString ks = ksb.getShallowKeyString();
         rc = ks.getOwned();
         ASSERT_EQ(SDB_OK, rc);
         bson::BSONObj objFromKey = ks.toBSON(pattern, TRUE);
         EXPECT_EQ(obj.woCompare(objFromKey), 0);
         bsb.reset();
         ksb.reset();
      }
   }

   TEST_F(key_string_test, base_double)
   {
      INT32 rc = SDB_OK;
      bson::BSONObjBuilder bsb;
      bsb.appendNumber("a", 1);
      bsb.appendNumber("b", -1);
      bson::BSONObj pattern = bsb.obj();
      bsb.reset();
      vector<vector<FLOAT64>> numbers = {
          {std::numeric_limits<FLOAT64>::denorm_min(),
           std::numeric_limits<FLOAT64>::denorm_min()},
          {-172.8 - 172.8},
          {-172.5, -172.5},
          {-172.3, -172.3},
          {-0.0, -0.0},
          {0.0, 0.0},
          {172.3, 172.3},
          {172.5, 172.5},
          {172.8, -172.8},
          {minLargeFloat64, minLargeFloat64},
          {std::numeric_limits<FLOAT64>::max(),
           std::numeric_limits<FLOAT64>::max()}};
      for (UINT32 i = 0; i < numbers.size(); i++)
      {
         bsb.appendNumber("a", static_cast<FLOAT64>(numbers[i][0]));
         bsb.appendNumber("b", static_cast<FLOAT64>(numbers[i][1]));
         orderingWrapper ord(0b10, 2);
         bson::BSONObj obj = bsb.obj();
         rc = ksb.appendAllElements(obj, ord);
         ASSERT_EQ(SDB_OK, rc);
         rc = ksb.done();
         ASSERT_EQ(SDB_OK, rc);
         keyString ks = ksb.getShallowKeyString();
         rc = ks.getOwned();
         ASSERT_EQ(SDB_OK, rc);
         bson::BSONObj objFromKey = ks.toBSON(pattern, TRUE);
         EXPECT_EQ(obj.woCompare(objFromKey), 0);
         bsb.reset();
         ksb.reset();
      }
   }

   TEST_F(key_string_test, base_decimal)
   {
      INT32 rc = SDB_OK;
      bson::BSONObjBuilder bsb;
      bsb.appendNumber("a", 1);
      bsb.appendNumber("b", -1);
      bson::BSONObj pattern = bsb.obj();
      bsb.reset();
      vector<vector<string>> numbers = {
          {"-172172.839651", "-172172.839651"},
          {"-172.8", "-172.8"},
          {"-172.5", "-172.5"},
          {"-172.3", "-172.3"},
          {"-0.0", "-0.0"},
          {"0.0", "0.0"},
          {"172.3", "172.3"},
          {"172.5", "172.5"},
          {"172.8", "172.8"},
          {"172172.839651", "172172.839651"},
          {to_string(minLargeFloat64), to_string(minLargeFloat64)}};
      for (UINT32 i = 0; i < numbers.size(); i++)
      {
         bsb.appendDecimal("a", numbers[i][0]);
         bsb.appendDecimal("b", numbers[i][1]);
         orderingWrapper ord(0b10, 2);
         bson::BSONObj obj = bsb.obj();
         rc = ksb.appendAllElements(obj, ord);
         ASSERT_EQ(SDB_OK, rc);
         rc = ksb.done();
         ASSERT_EQ(SDB_OK, rc);
         keyString ks = ksb.getShallowKeyString();
         rc = ks.getOwned();
         ASSERT_EQ(SDB_OK, rc);
         bson::BSONObj objFromKey = ks.toBSON(pattern, TRUE);
         // std::cout<< i << endl << obj.toString(0,0,0) << endl <<
         // objFromKey.toString(0,0,0) <<endl;
         EXPECT_EQ(obj.woCompare(objFromKey), 0);
         bsb.reset();
         ksb.reset();
      }
   }

   TEST_F(key_string_test, base_string)
   {
      INT32 rc = SDB_OK;
      orderingWrapper ord(0b10, 2);
      bsb.appendStrWithNoTerminating("a", "foo bar", 7);
      bsb.appendStrWithNoTerminating("b", "foo bar", 7);
      rc = ksb.appendAllElements(bsb.done(), ord);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
      BSONObj objFromKey = ks.toBSON(BSON("a" << 1 << "b" << -1), TRUE);
      // std::cout<< bsb.done().toString(0,0,0) <<endl
      // <<objFromKey.toString(0,0,0) << endl;
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
   }

   TEST_F(key_string_test, base_symbol)
   {
      INT32 rc = SDB_OK;
      orderingWrapper ord(0b10, 2);
      bsb.appendSymbol("a", {"foo\0bar", 7});
      bsb.appendSymbol("b", {"foo\0bar", 7});
      rc = ksb.appendAllElements(bsb.done(), ord);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
      BSONObj objFromKey = ks.toBSON(BSON("a" << 1 << "b" << -1), TRUE);
      // std::cout<< bsb.done().toString(0,0,0) <<endl
      // <<objFromKey.toString(0,0,0) << endl;
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
   }

   TEST_F(key_string_test, base_object)
   {
      INT32 rc = SDB_OK;
      bsb.appendNumber("int", 1);
      bsb.appendNumber("object", 1);
      bsb.appendNumber("double", 1);
      bson::BSONObj pattern = bsb.obj();
      bsb.reset();

      bsb.appendNumber("x_int", 172);
      bsb.appendNumber("x_long", static_cast<INT64>(-172));
      bsb.appendNumber("y", 172.5);
      bsb.appendDecimal("z", "172.3");
      bson::BSONObj subObj = bsb.obj();
      bsb.reset();

      orderingWrapper ord(0, 3);
      bsb.appendNumber("int", 172);
      bsb.appendObject("object", subObj.objdata());
      bsb.appendNumber("double", 172.5);
      bson::BSONObj obj = bsb.obj();
      rc = ksb.appendAllElements(obj, ord);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
      rc = ks.getOwned();
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj objFromKey = ks.toBSON(pattern, TRUE);
      // std::cout<< endl << obj.toString(0,0,0) << endl <<
      //   objFromKey.toString(0,0,0) <<endl;
      EXPECT_EQ(objFromKey.woCompare(obj), 0);
   }

   TEST_F(key_string_test, base_array)
   {
      INT32 rc = SDB_OK;
      orderingWrapper ord(0b10, 2);
      BSONArrayBuilder bsab;
      bsonDecimal dec;
      dec.fromString("172.3");
      bsab.append(172);
      bsab.append(static_cast<INT64>(-172));
      bsab.append(172.5);
      bsab.append(dec);
      BSONArray arr = bsab.arr();
      bsb.appendArray("array", arr);
      bsb.appendArray("array_desc", arr);
      rc = ksb.appendAllElements(bsb.done(), ord);
      ASSERT_EQ(SDB_OK, rc);
      // std::cout << endl << bsb.done().toString(0, 0, 0) << endl;
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
      rc = ks.getOwned();
      ASSERT_EQ(SDB_OK, rc);
      bson::BSONObj objFromKey =
          ks.toBSON(BSON("array" << 1 << "array_desc" << -1), TRUE);
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
   }

   TEST_F(key_string_test, base_bindata)
   {
      INT32 rc = SDB_OK;
      orderingWrapper o(0b111111000000, 12);
      const CHAR *byteArray = "foobar";
      bsb.appendBinData("BinDataGeneral",
                        strlen(byteArray),
                        bson::BinDataType::BinDataGeneral,
                        byteArray);
      bsb.appendBinData("Function",
                        strlen(byteArray),
                        bson::BinDataType::Function,
                        byteArray);
      bsb.appendBinData("ByteArrayDeprecated",
                        strlen(byteArray),
                        bson::BinDataType::ByteArrayDeprecated,
                        byteArray);
      bsb.appendBinData(
          "bdtUUID", strlen(byteArray), bson::BinDataType::bdtUUID, byteArray);
      bsb.appendBinData(
          "MD5Type", strlen(byteArray), bson::BinDataType::MD5Type, byteArray);
      bsb.appendBinData("bdtCustom",
                        strlen(byteArray),
                        bson::BinDataType::bdtCustom,
                        byteArray);

      bsb.appendBinData("BinDataGeneral",
                        strlen(byteArray),
                        bson::BinDataType::BinDataGeneral,
                        byteArray);
      bsb.appendBinData("Function",
                        strlen(byteArray),
                        bson::BinDataType::Function,
                        byteArray);
      bsb.appendBinData("ByteArrayDeprecated",
                        strlen(byteArray),
                        bson::BinDataType::ByteArrayDeprecated,
                        byteArray);
      bsb.appendBinData(
          "bdtUUID", strlen(byteArray), bson::BinDataType::bdtUUID, byteArray);
      bsb.appendBinData(
          "MD5Type", strlen(byteArray), bson::BinDataType::MD5Type, byteArray);
      bsb.appendBinData("bdtCustom",
                        strlen(byteArray),
                        bson::BinDataType::bdtCustom,
                        byteArray);

      rc = ksb.appendAllElements(bsb.done(), o);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
      ks.getOwned();
      bson::BSONObj objFromKey =
          ks.toBSON(BSON("BinDataGeneral"
                         << 1 << "Function" << 1 << "ByteArrayDeprecated" << 1
                         << "bdtUUID" << 1 << "MD5Type" << 1 << "bdtCustom" << 1
                         << "BinDataGeneral" << -1 << "Function" << -1
                         << "ByteArrayDeprecated" << -1 << "bdtUUID" << -1
                         << "MD5Type" << -1 << "bdtCustom" << -1),
                    TRUE);
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
      // std::cout << bsb.done().toString(0, 0, 0) << endl
      //          << objFromKey.toString(0, 0, 0) << endl;
   }

   TEST_F(key_string_test, base_oid)
   {
      INT32 rc = SDB_OK;
      orderingWrapper ord(0b10, 2);
      bsb.appendOID("a", nullptr, TRUE);
      bsb.appendOID("b", nullptr, TRUE);
      rc = ksb.appendAllElements(bsb.done(), ord);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
      BSONObj objFromKey = ks.toBSON(BSON("a" << 1 << "b" << -1), TRUE);
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
   }

   TEST_F(key_string_test, base_bool)
   {
      INT32 rc = SDB_OK;
      orderingWrapper ord(0b1010, 4);
      bsb.appendBool("false_asc", FALSE);
      bsb.appendBool("false_desc", FALSE);
      bsb.appendBool("true_asc", TRUE);
      bsb.appendBool("true_desc", TRUE);
      rc = ksb.appendAllElements(bsb.done(), ord);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
      BSONObj objFromKey =
          ks.toBSON(BSON("false_asc" << 1 << "false_desc" << -1 << "true_asc"
                                     << 1 << "true_desc" << -1),
                    TRUE);
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
   }

   TEST_F(key_string_test, base_date)
   {
      orderingWrapper ord(0b10, 2);
      Date_t dt(2132134321);
      bsb.appendDate("a", dt);
      bsb.appendDate("b", dt);
      ksb.appendAllElements(bsb.done(), ord);
      ksb.done();
      keyString ks = ksb.getShallowKeyString();
      BSONObj objFromKey = ks.toBSON(BSON("a" << 1 << "b" << -1), TRUE);
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
   }

   TEST_F(key_string_test, base_timestamp)
   {
      orderingWrapper ord(0b10, 2);
      bsb.appendTimestamp("a", 72192821020281);
      bsb.appendTimestamp("b", 72192821020281);
      ksb.appendAllElements(bsb.done(), ord);
      ksb.done();
      keyString ks = ksb.getShallowKeyString();
      BSONObj objFromKey = ks.toBSON(BSON("a" << 1 << "b" << -1), TRUE);
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
   }

   TEST_F(key_string_test, base_regex)
   {
      orderingWrapper ord(0b10, 2);
      StringData pattern("^.*[]?");
      StringData flags("foobar");
      bsb.appendRegex("a", pattern, flags);
      bsb.appendRegex("b", pattern, flags);
      ksb.appendAllElements(bsb.done(), ord);
      ksb.done();
      keyString ks = ksb.getShallowKeyString();
      BSONObj objFromKey = ks.toBSON(BSON("a" << 1 << "b" << -1), TRUE);
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
   }

   TEST_F(key_string_test, base_dbref)
   {
      orderingWrapper ord(0b10, 2);
      StringData ns = "db1.cl1";
      OID oid = OID::gen();
      bsb.appendDBRef("a", ns, oid);
      bsb.appendDBRef("b", ns, oid);
      ksb.appendAllElements(bsb.done(), ord);
      ksb.done();
      keyString ks = ksb.getShallowKeyString();
      BSONObj objFromKey = ks.toBSON(BSON("a" << 1 << "b" << -1), TRUE);
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
   }

   TEST_F(key_string_test, base_code)
   {
      orderingWrapper ord(0b10, 2);
      StringData code = "()=>{}";
      bsb.appendCode("a", code);
      bsb.appendCode("b", code);
      ksb.appendAllElements(bsb.done(), ord);
      ksb.done();
      keyString ks = ksb.getShallowKeyString();
      BSONObj objFromKey = ks.toBSON(BSON("a" << 1 << "b" << -1), TRUE);
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
   }

   TEST_F(key_string_test, base_codewscope)
   {
      orderingWrapper ord(0b10, 2);
      StringData code = "()=>{}";
      BSONObj scope = BSON("1" << 1);
      bsb.appendCodeWScope("a", code, scope);
      bsb.appendCodeWScope("b", code, scope);
      ksb.appendAllElements(bsb.done(), ord);
      ksb.done();
      keyString ks = ksb.getShallowKeyString();
      BSONObj objFromKey = ks.toBSON(BSON("a" << 1 << "b" << -1), TRUE);
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
   }

   TEST_F(key_string_test, base_maxkey)
   {
      orderingWrapper ord(0b10, 2);
      bsb.appendMaxKey("a");
      bsb.appendMaxKey("b");
      ksb.appendAllElements(bsb.done(), ord);
      ksb.done();
      keyString ks = ksb.getShallowKeyString();
      BSONObj objFromKey = ks.toBSON(BSON("a" << 1 << "b" << -1), TRUE);
      EXPECT_EQ(objFromKey.woCompare(bsb.done()), 0);
   }

   TEST_F(key_string_test, base_discriminator)
   {
      orderingWrapper ord(0, 2);
      bson::BSONObjBuilder bsb;
      bsb.appendNumber("a", -1);
      ksb.appendAllElements(bsb.done(), ord, Discriminator::EXCLUSIVE_BEFORE);
      ksb.done();
      keyString lessQuery = ksb.getShallowKeyString();
      lessQuery.getOwned();
      ksb.reset();
      bsb.reset();

      bsb.appendNumber("a", -1);
      ksb.appendAllElements(bsb.done(), ord, Discriminator::EXCLUSIVE_AFTER);
      ksb.done();
      keyString greaterQuery = ksb.getShallowKeyString();
      greaterQuery.getOwned();
      ksb.reset();
      bsb.reset();
      vector<vector<int>> numbers = {{-172, 5},
                                     {-1, -3},
                                     {-1, -2},
                                     {-1, -1},
                                     {0, 1},
                                     {0, 2},
                                     {0, 3},
                                     {1, 1},
                                     {1, 2},
                                     {1, 3},
                                     {2, 5}};

      for (UINT32 i = 0; i < numbers.size(); i++)
      {
         bsb.appendNumber("a", numbers[i][0]);
         bsb.appendNumber("b", numbers[i][1]);
         bson::BSONObj obj = bsb.obj();
         ksb.appendAllElements(obj, ord);
         ksb.done();
         keyString ks = ksb.getShallowKeyString();
         ks.getOwned();
         UINT32 comparableSize =
             ks.getComparableSize() < lessQuery.getComparableSize()
                 ? ks.getComparableSize()
                 : lessQuery.getComparableSize();
         if (i < 1)
         {
            EXPECT_LE(ossMemcmp(ks.getDataSlice().data(),
                                lessQuery.getDataSlice().data(),
                                comparableSize),
                      0);
         }
         else
         {
            EXPECT_GT(ossMemcmp(ks.getDataSlice().data(),
                                lessQuery.getDataSlice().data(),
                                comparableSize),
                      0);
         }
         if (i < 4)
         {
            EXPECT_LE(ossMemcmp(ks.getDataSlice().data(),
                                greaterQuery.getDataSlice().data(),
                                comparableSize),
                      0);
         }
         else
         {
            EXPECT_GT(ossMemcmp(ks.getDataSlice().data(),
                                greaterQuery.getDataSlice().data(),
                                comparableSize),
                      0);
         }
         bsb.reset();
         ksb.reset();
      }
   }

   TEST_F(key_string_test, base_ahead_key)
   {
      INT32 rc = SDB_OK;
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
         ASSERT_LT(ossMemcmp(buf + i * 4, buf + (i + 1) * 4, 4), 0);
      }
   }

   TEST_F(key_string_test, base_cmp_decimal)
   {
      INT32 rc = SDB_OK;
      bsb.appendDecimal("decimal", "172657.000003");
      bsb.appendDecimal("decimal", "0.000003");
      orderingWrapper ord(0, 32);
      rc = ksb.appendAllElements(bsb.obj(), ord);
      ASSERT_EQ(SDB_OK, rc);
      rc = ksb.done();
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
   }

   TEST_F(key_string_test, base_keyslice)
   {
      INT32 rc = SDB_OK;
      bson::BSONObj obj = BSON("a"
                               << "foobar");
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

      INT32 res = ks1.getKeySlice().compare(ks2.getKeySlice());
      ASSERT_EQ(res, 0);

      ksb1.reset();
      ksb2.reset();
      std::string str(200, 'x');
      bson::BSONObjBuilder ob;
      ob << "name" << str << "foo" << str << "int" << 123 << "ss" << str;
      obj = ob.obj();
      orderingWrapper ord(0, 4);
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

      res = ks1.getKeySlice().compare(ks2.getKeySlice());
      ASSERT_EQ(res, 0);
   }

   TEST_F(key_string_test, base_build_predicate)
   {
      INT32 rc = SDB_OK;
      orderingWrapper ord(0, 3);
      inclusiveVec iv;
      iv.setExclusive(1);
      BSONObj pattern = BSON("a" << 1 << "b" << 1 << "c" << 1);
      bsb.appendNumber("a", 2);
      bsb.appendNumber("b", 1.5);
      bsb.appendNumber("c", 1.2);
      BSONObj obj = bsb.obj();
      rc = ksb.buildPredicate(obj, ord, iv, TRUE);
      ASSERT_EQ(SDB_OK, rc);
      keyString ks1 = ksb.getShallowKeyString();
      ks1.getOwned();
      BSONObj predicate1 = ks1.toBSON(pattern, TRUE);
      EXPECT_EQ(predicate1.woCompare(BSON("a" << 2 << "b" << 1.5)), 0);
      ksb.reset();

      rc = ksb.buildPredicate(obj, ord, iv, FALSE);
      ASSERT_EQ(SDB_OK, rc);
      keyString ks2 = ksb.getShallowKeyString();
      ks2.getOwned();
      BSONObj predicate2 = ks2.toBSON(pattern, TRUE);
      EXPECT_EQ(predicate2.woCompare(BSON("a" << 2)), 0);
   }

   TEST_F(key_string_test, base_build_index_entry)
   {
      INT32 rc = SDB_OK;
      orderingWrapper ord(0, 2);
      bsb.appendNumber("a", 1);
      bsb.appendNumber("b", 1);
      BSONObj pattern = bsb.obj();
      bsb.reset();

      bsb.appendNumber("x", 1);
      bsb.appendNumber("y", 1.5);
      BSONObj subObj = bsb.obj();
      bsb.reset();

      bsb.appendNumber("a", 1);
      bsb.appendObject("b", subObj.objdata());
      BSONObj obj = bsb.obj();
      keyStringBuilder<> ksb;
      recordID rid(32, 2);
      globalIndexID indexId(1, 10, 2);
      UINT64 lsn = 1245;
      rc = ksb.buildIndexEntryKey(obj, ord, rid, &indexId, &lsn);
      ASSERT_EQ(SDB_OK, rc);
      keyString ks = ksb.getShallowKeyString();
      ks.getOwned();
      BSONObj objFromKey = ks.toBSON(pattern, TRUE);
      EXPECT_EQ(objFromKey.woCompare(obj), 0);
   }

} // namespace vessel
} // namespace engine
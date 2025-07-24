/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = randomBsonGenerator.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/18/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_RANDOM_BSON_GENERATOR_H_
#define VESSEL_RANDOM_BSON_GENERATOR_H_


#include "../bson/bsonDecimal.h"
#include "..//bson/bsonobj.h"
#include "../bson/bsonobjbuilder.h"
#include <random>

namespace engine
{
namespace vessel
{
   class randomBsonGenerator : public SDBObject
   {
   public:
      randomBsonGenerator()
      {
         generator.seed(seed());
      }

   public:
      bson::BSONType randomBsonType()
      {
         std::uniform_int_distribution<UINT32> uint32_distrib;
         constexpr bson::BSONType bsonTypeCandidate[] = {bson::MinKey,
                                                   /*EOO,*/ bson::NumberDouble,
                                                   bson::String,
                                                   bson::Object,
                                                   bson::Array,
                                                   bson::BinData,
                                                   bson::Undefined,
                                                   bson::jstOID,
                                                   bson::Bool,
                                                   bson::Date,
                                                   bson::jstNULL,
                                                   bson::RegEx,
                                                   bson::DBRef,
                                                   bson::Code,
                                                   bson::Symbol,
                                                   // CodeWScope,
                                                   bson::NumberInt,
                                                   bson::Timestamp,
                                                   bson::NumberLong,
                                                   bson::NumberDecimal,
                                                   bson::MaxKey};
         return bsonTypeCandidate[random<UINT32>(
             0, (sizeof(bsonTypeCandidate) / sizeof(bson::BSONType)) - 1)];
      }

      bson::BinDataType randomBinDataType()
      {
         constexpr bson::BinDataType binDataTypeCandidate[] = {bson::BinDataGeneral,
                                                         bson::Function,
                                                         bson::ByteArrayDeprecated,
                                                         bson::bdtUUID,
                                                         bson::MD5Type,
                                                         bson::bdtCustom};
         return binDataTypeCandidate[random<UINT32>(
             0, (sizeof(binDataTypeCandidate) / sizeof(bson::BinDataType)) - 1)];
      }

      FLOAT64 randomFloat64()
      {
         std::uniform_real_distribution<FLOAT64> float_distrib;
         return float_distrib(generator);
      }

      template <typename T>
      T random(T T_min = 0, T T_max = numeric_limits<T>::max())
      {
         std::uniform_int_distribution<T> distrib(T_min, T_max);
         return distrib(generator);
      }

      std::string randomString(UINT32 maxLen = 50)
      {
         std::string output;
         CHAR buf[maxLen];
         for (UINT32 i = 0; i < maxLen; i++)
         {
            buf[i] = random<UINT8>(1);
         }
         output.append(buf, random<UINT32>(1, maxLen));
         return output;
      }

      void randomToAssign(CHAR* buf, UINT32 size)
      {
         for(UINT32 i = 0;i < size ;i++)
         {
            buf[i] = random<CHAR>();
         }
      }

      vector<bson::BSONType> randomTypes(UINT32 nkeys)
      {

         vector<bson::BSONType> v;
         for (UINT32 i = 0; i < nkeys; i++)
         {
            v.push_back(randomBsonType());
         }
         return v;
      }

      INT64 randomSeconds()
      {
         std::uniform_int_distribution<INT64> distrib(1);
         return distrib(generator);
      }

      INT32 randomMicroseconds()
      {
         std::uniform_int_distribution<INT32> distrib(1, 999999);
         return distrib(generator);
      }

      bson::BSONObj randomBson(vector<bson::BSONType> typeList, INT32 depth = 3)
      {
         bson::BSONObjBuilder bsb;
         for (UINT32 i = 0; i < typeList.size(); i++)
         {
            bson::BSONType t = typeList[i];

            std::string str = std::to_string(i);
            const CHAR *fieldName = str.c_str();
            switch (t)
            {
            case bson::MinKey:
               bsb.appendMinKey(fieldName);
               break;
            case bson::EOO:
               bsb.appendNull(fieldName);
               break;
            case bson::NumberDouble:
               bsb.appendNumber(fieldName, randomFloat64());
               break;
            case bson::String: {
               std::string s = randomString();
               bsb.appendStrWithNoTerminating(fieldName, s.data(), s.size());
               break;
            }
            case bson::Object:
            case bson::Array: {
               if (depth > 0)
               {
                  bson::BSONObj obj = randomBson(random<UINT32>(1, 5), depth - 1);
                  bsb.appendObject(fieldName, obj.objdata(), obj.objsize());
               }
               else
               {
                  bsb.appendNull(fieldName);
               }
               break;
            }
            case bson::BinData: {
               std::string s = randomString();
               bsb.appendBinData(
                   fieldName, s.size(), randomBinDataType(), s.data());
               break;
            }
            case bson::Undefined:
               bsb.appendUndefined(fieldName);
               break;
            case bson::jstOID:
               bsb.appendOID(fieldName, nullptr, TRUE);
               break;
            case bson::Bool:
               bsb.appendBool(fieldName, random<UINT32>(0, 1));
               break;
            case bson::Date: {
               bson::Date_t dt(random<INT64>());
               bsb.appendDate(fieldName, dt);
               break;
            }
            case bson::jstNULL:
               bsb.appendNull(fieldName);
               break;
            case bson::RegEx: {
               std::string regex = randomString();
               std::string flags = randomString();
               bsb.appendRegex(fieldName, regex, flags);
               break;
            }
            case bson::DBRef: {
               std::string ns = randomString(20);
               bson::OID oid;
               oid.init();
               bsb.appendDBRef(fieldName, ns, oid);
               break;
            }
            case bson::Code: {
               std::string code = randomString(20);
               bsb.appendCode(fieldName, code);
               break;
            }
            case bson::Symbol: {
               std::string symbol = randomString();
               bsb.appendSymbol(fieldName, symbol);
               break;
            }
            case bson::CodeWScope: {
               std::string code = randomString(20);
               bson::BSONObj scope = BSON("0" << random<INT32>());
               bsb.appendCodeWScope(fieldName, code, scope);
               break;
            }
            case bson::NumberInt:
               bsb.appendNumber(fieldName, random<INT32>());
               break;
            case bson::Timestamp:
               bsb.appendTimestamp(
                   fieldName, random<INT32>(1) * 1000, randomMicroseconds());
               break;
            case bson::NumberLong:
               bsb.appendNumber(fieldName, random<INT64>());
               break;
            case bson::NumberDecimal: {
               bson::bsonDecimal dec;
               if (random<INT32>(0, 1) == 1)
               {
                  dec.fromDouble(randomFloat64());
               }
               else
               {
                  std::string decStr;
                  if (random<INT32>(0, 1) == 1)
                  {
                     decStr.append("-");
                  }

                  if (random<INT32>(0, 1) == 1)
                  {
                     decStr.append(std::to_string(random<UINT64>()));
                     if (random<INT32>(0, 1) == 1)
                     {
                        decStr.append(".");
                        decStr.append(std::to_string(random<UINT64>()));
                     }
                  }
                  else
                  {
                     decStr.append("0.");
                     decStr.append(std::to_string(random<UINT64>()));
                  }

                  dec.fromString(decStr.c_str());
               }
               bsb.append(fieldName, dec);
               break;
            }
            case bson::MaxKey:
               bsb.appendMaxKey(fieldName);
               break;
            default:
               SDB_ASSERT(FALSE, "Unexpected bson type");
               break;
            }
         }
         return bsb.obj();
      }

      bson::BSONObj randomBson(UINT32 nkeys, INT32 depth = 3)
      {
         vector<bson::BSONType> typeList = randomTypes(nkeys);
         return randomBson(typeList, depth);
      }

      random_device seed;
      std::default_random_engine generator;
   };
} // namespace vessel
} // namespace engine

#endif
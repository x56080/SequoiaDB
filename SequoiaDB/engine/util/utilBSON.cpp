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

   
*******************************************************************************/
#include <utilBSON.hpp>

#include <string>

#include <ossTypes.h>

#include <../bson/bson.h>

using std::string;

namespace
{

/*
 * Getters - no exceptions can be thrown. Assume the value type is correct.
 */

// Generic getter. Use the BSONElement::Val() as the getter.
template <typename T> void _valGetter(bson::BSONElement &ele, T *pOutput)
{
   ele.Val(*pOutput);
}

// Need to convert different number types safely so these are required
template <> void _valGetter<INT32>(bson::BSONElement &ele, INT32 *pOutput)
{
   *pOutput = ele.numberInt();
}
template <> void _valGetter<INT64>(bson::BSONElement &ele, INT64 *pOutput)
{
   *pOutput = ele.numberLong();
}
// BSON doesn't differentiate unsigned
template <> void _valGetter<UINT32>(bson::BSONElement &ele, UINT32 *pOutput)
{
   *pOutput = ele.numberInt();
}
template <> void _valGetter<UINT64>(bson::BSONElement &ele, UINT64 *pOutput)
{
   *pOutput = ele.numberLong();
}

/*
 * Validators - return non-zero if not the right type.
 */

BOOLEAN _checkNum(const bson::BSONElement &ele) { return ele.isNumber(); }

BOOLEAN _checkStr(const bson::BSONElement &ele)
{
   return ele.type() == bson::String;
}

BOOLEAN _checkObj(const bson::BSONElement &ele) { return ele.isABSONObj(); }

// Base fromBsonObj. Callers pass the validator and getter functions
template <typename T, typename V, typename G>
INT32 _fromBsonObj(const bson::BSONObj &input, const string &field, T *pOutput,
                   BOOLEAN required, V validator, G getter)
{
   INT32 rc = SDB_OK;
   bson::BSONElement ele = input.getField(field);
   if (ele.eoo())
   {
      if (required)
      {
         return (rc = SDB_FIELD_NOT_EXIST);
      }
      return rc;
   }
   if (!validator(ele))
   {
      return (rc = SDB_INVALIDARG);
   }
   getter(ele, pOutput);
   return rc;
}

// Generic getter fromBsonObj. Callers pass a validator. Uses the default
// getter.
template <typename T, typename V>
INT32 _fromBsonObj(const bson::BSONObj &input, const string &field, T *pOutput,
                   BOOLEAN required, V validator)
{
   return _fromBsonObj(input, field, pOutput, required, validator,
                       _valGetter<T>);
}

} // anonymous namespace

namespace engine
{

namespace util
{

/*
 * Explicit specializations
 */

// INT32
template <>
INT32 fromBsonObj<INT32>(const bson::BSONObj &input, const string &field,
                         INT32 *pOutput, BOOLEAN required)
{
   return _fromBsonObj(input, field, pOutput, required, _checkNum);
}

// UINT32
template <>
INT32 fromBsonObj<UINT32>(const bson::BSONObj &input, const string &field,
                          UINT32 *pOutput, BOOLEAN required)
{
   return _fromBsonObj(input, field, (INT32 *)pOutput, required, _checkNum);
}

// INT64
template <>
INT32 fromBsonObj<INT64>(const bson::BSONObj &input, const string &field,
                         INT64 *pOutput, BOOLEAN required)
{
   return _fromBsonObj(input, field, pOutput, required, _checkNum);
}

// UINT64
template <>
INT32 fromBsonObj<UINT64>(const bson::BSONObj &input, const string &field,
                          UINT64 *pOutput, BOOLEAN required)
{
   return _fromBsonObj(input, field, (INT64 *)pOutput, required, _checkNum);
}

// String
template <>
INT32 fromBsonObj<string>(const bson::BSONObj &input, const string &field,
                          string *pOutput, BOOLEAN required)
{
   return _fromBsonObj(input, field, pOutput, required, _checkStr);
}

// Sub-object
template <>
INT32 fromBsonObj<bson::BSONObj>(const bson::BSONObj &input,
                                 const string &field, bson::BSONObj *pOutput,
                                 BOOLEAN required)
{
   return _fromBsonObj(input, field, pOutput, required, _checkObj);
}

INT32 boolFromBsonObj(const bson::BSONObj &input, const string &field,
                      BOOLEAN *pOutput, BOOLEAN required)
{
   INT32 rc = SDB_OK;
   bson::BSONElement ele = input.getField(field);
   *pOutput = ele.trueValue();
   return rc;
}

} // namespace util

} // namespace engine


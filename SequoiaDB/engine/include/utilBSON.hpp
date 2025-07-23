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
#ifndef UTIL_BSON_HPP__
#define UTIL_BSON_HPP__

#include <string>

#include <ossTypes.h>

#include <../bson/bson.h>

namespace engine {

namespace util {

// Note that there are 2 versions of each until string_view is available.
// const char* is preferable if the caller passes a C-style string as it avoids
// creating a temporary string, but it is incompatible with a string input.
// const string& is efficient for string inputs as it avoids copy.
// string_view would be compatible and efficient for both.

// Extract the field's value as the type of output.
// @param   input    Object from which to extract
// @param   field    Name of the field in the object
// @param   pOutput  Pointer to the variable to hold the output if successful
// @param   required Changes behaviour when field is not found
// @return  SDB_FIELD_NOT_EXIST if the field is not found in the input AND
//                              required is true
//          SDB_INVALIDARG if the field value does not map to the output type
//          SDB_OK otherwise
template <typename T>
INT32 fromBsonObj(const bson::BSONObj &input, const char *field, T *pOutput,
                  BOOLEAN required = TRUE);

template <typename T>
INT32 fromBsonObj(const bson::BSONObj &input, const string &field, T *pOutput,
                  BOOLEAN required = TRUE)
{
   return fromBsonObj(input, field.c_str(), pOutput, required);
}

// Explicit for boolean values. Sets pOutput to 1 or 0. This is needed because
// sdb BOOLEAN is a typedef of INT32 so the template would interpret the value
// as a number.
INT32 boolFromBsonObj(const bson::BSONObj &input, const char *field,
                      BOOLEAN *pOutput, BOOLEAN required = TRUE);

INT32 boolFromBsonObj(const bson::BSONObj &input, const string &field,
                      BOOLEAN *pOutput, BOOLEAN required = TRUE);

} // namespace util

} // namespace engine

#endif // UTIL_BSON_HPP__


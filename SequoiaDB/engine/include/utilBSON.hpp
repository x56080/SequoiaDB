/*******************************************************************************

   Copyright (C) 2011-2020 SequoiaDB Ltd.

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

*******************************************************************************/

#ifndef UTIL_BSON_HPP__
#define UTIL_BSON_HPP__

#include <string>

#include <ossTypes.h>

#include <../bson/bson.h>

namespace engine {

namespace util {

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
INT32 fromBsonObj(const bson::BSONObj &input, const string &field, T *pOutput,
                  BOOLEAN required = TRUE);

} // namespace util

} // namespace engine

#endif // UTIL_BSON_HPP__


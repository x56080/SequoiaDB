/*******************************************************************************
   Copyright (C) 2011-2018 SequoiaDB Ltd.

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


/** \file utilTypeCast.h
    \brief type cast.
*/
#ifndef UTIL_TYPE_CAST__H
#define UTIL_TYPE_CAST__H

#include "core.h"

typedef union
{
   INT32    intVal ;
   INT64    longVal ;
   FLOAT64  doubleVal ;
} utilNumberVal ;

SDB_EXTERN_C_START

SDB_EXPORT INT32 utilStrToNumber( const CHAR* data, INT32 length,
                                  INT32 *type, utilNumberVal *value,
                                  INT32 *valueLength ) ;

SDB_EXTERN_C_END

#endif // end UTIL_TYPE_CAST__H
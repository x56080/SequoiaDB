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

   Source File Name = spd.h

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
/** \file spd.h
    \brief Js return type for stored procedures used in sdb shell
*/

#ifndef SPD_H_
#define SPD_H_

#include "core.h"

/*
   _SDB_SPD_RES_TYPE define
*/
enum _SDB_SPD_RES_TYPE
{
   SDB_SPD_RES_TYPE_VOID = 0, /**< Js return void type */
   SDB_SPD_RES_TYPE_STR,      /**< Js return a string */
   SDB_SPD_RES_TYPE_NUMBER,   /**< Js return a number */
   SDB_SPD_RES_TYPE_OBJ,      /**< Js return an object */
   SDB_SPD_RES_TYPE_BOOL,     /**< Js return a bool */
   SDB_SPD_RES_TYPE_RECORDSET,/**< Js return a cursor handle */
   SDB_SPD_RES_TYPE_SPECIALOBJ = 10, /**< Js return a special obj */
   SDB_SPD_RES_TYPE_NULL,     /**< Js return null type */

   SDB_SPD_RES_TYPE_MAX
} ;

/** Js return type */
typedef enum _SDB_SPD_RES_TYPE SDB_SPD_RES_TYPE ;

#endif //SPD_H_


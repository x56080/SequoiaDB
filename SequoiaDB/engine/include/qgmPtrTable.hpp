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

   Source File Name = qgmPtrTable.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef QGMPTRTABLE_HPP_
#define QGMPTRTABLE_HPP_

#include "qgmDef.hpp"
#include <map>
#include <set>
#include "ossMem.hpp"

namespace engine
{
   /*
      qgm_char_cmp define
   */
   struct qgm_char_cmp
   {
      bool operator()( const CHAR *a, const CHAR *b )
      {
         return ossStrcmp( a, b ) < 0 ;
      }
   } ;

   typedef std::set<qgmField>                               PTR_TABLE ;
   typedef std::map<const CHAR*, UINT32, qgm_char_cmp>      STR_TABLE ;
#if defined (_WINDOWS)
   typedef STR_TABLE::iterator                              STR_TABLE_IT ;
#else
   typedef std::map<const CHAR*, UINT32>::iterator          STR_TABLE_IT ;
#endif // _WINDOWS

   /*
      _qgmPtrTable define
   */
   class _qgmPtrTable : public SDBObject
   {
   public:
      _qgmPtrTable() ;
      virtual ~_qgmPtrTable() ;

   public:
      INT32 getField( const SQL_CON_ITR &itr,
                      qgmField &field ) ;

      INT32 getField( const CHAR *begin, UINT32 size,
                      qgmField &field ) ;

      INT32 getOwnField( const CHAR *begin, qgmField &field ) ;
      INT32 getOwnField( const CHAR *begin, UINT32 size,
                         qgmField &field ) ;

      INT32 getAttr( const SQL_CON_ITR &itr,
                     qgmDbAttr &attr ) ;

      INT32 getAttr( const CHAR *begin, UINT32 size,
                     qgmDbAttr &attr ) ;

      INT32 getOwnAttr( const CHAR *begin, UINT32 size,
                        qgmDbAttr &attr ) ;

      INT32 getUniqueFieldAlias( qgmField &field ) ;
      INT32 getUniqueTableAlias( qgmField &field ) ;

      qgmField    getField( const qgmField &sub1,
                            const qgmField &sub2 ) ;
      const CHAR* getOwnedString( const CHAR *str ) ;

   private:
      PTR_TABLE _table ;
      STR_TABLE _stringTable ;
      UINT32    _uniqueFieldID ;
      UINT32    _uniqueTableID ;

   } ;

   typedef class _qgmPtrTable qgmPtrTable ;
}

#endif // QGMPTRTABLE_HPP_


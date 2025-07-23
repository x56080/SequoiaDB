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

   Source File Name = qgmPlInsert.hpp

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
#ifndef QGMPLINSERT_HPP_
#define QGMPLINSERT_HPP_

#include "qgmPlan.hpp"
#include "msgDef.h"

namespace engine
{
   class _qgmPlInsert : public _qgmPlan
   {
   public:
      _qgmPlInsert( const qgmDbAttr &collection,
                    const BSONObj &record = BSONObj() ) ;

      virtual ~_qgmPlInsert() ;

   public:
      virtual string toString() const ;

      virtual BOOLEAN needRollback() const ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext ( qgmFetchOut &next ) ;

      OSS_INLINE BOOLEAN _directInsert() const
      {
         return _input.size() == 0 ? TRUE : FALSE ;
      }

      INT32 _nextRecord( _pmdEDUCB *eduCB, BSONObj &obj ) ;

   protected:
      INT32 _insertVCS( const CHAR *fullName,
                        const BSONObj &insertor,
                        _pmdEDUCB *cb ) ;

   private:
      string         _fullName ;
      BSONObj        _insertor ;
      BOOLEAN        _got ;
      SDB_ROLE       _role ;
   } ;

   typedef class _qgmPlInsert qgmPlInsert ;
}

#endif


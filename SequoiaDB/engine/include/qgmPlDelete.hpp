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

   Source File Name = qgmPlDelete.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef QGMPLDELETE_HPP_
#define QGMPLDELETE_HPP_

#include "qgmPlan.hpp"
#include "utilInsertResult.hpp"

namespace engine
{
   struct _qgmConditionNode ;

   class _qgmPlDelete : public _qgmPlan
   {
   public:
      _qgmPlDelete( const qgmDbAttr &collection,
                    _qgmConditionNode *condition ) ;
      virtual ~_qgmPlDelete() ;
   public:
      virtual string toString() const ;
      virtual BOOLEAN needRollback() const ;
      virtual BOOLEAN canUseTrans() const { return TRUE ; }
      void buildRetInfo( BSONObjBuilder &builder ) const ;
      virtual void    setClientVersion( INT32 version ) ;
      virtual INT32   getCatalogVersion() const ;

   private:
      virtual INT32 _execute( _pmdEDUCB *eduCB ) ;

      virtual INT32 _fetchNext( qgmFetchOut &next, _pmdEDUCB *eduCB )
      {
         return SDB_SYS ;
      }

   protected:
      INT32 _deleteVCS( const CHAR *fullName,
                        const BSONObj &deletor,
                        _pmdEDUCB *cb ) ;

   private:
      qgmDbAttr         _collection ;
      BSONObj           _condition ;
      utilDeleteResult  _delResult ;
      INT32             _clientVersion ;
      INT32             _catalogVersion ;
   } ;

   typedef class _qgmPlDelete qgmPlDelete ;
}

#endif


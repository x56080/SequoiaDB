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

   Source File Name = qgmOptiUpdate.hpp

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
#ifndef QGMOPTIUPDATE_HPP_
#define QGMOPTIUPDATE_HPP_

#include "qgmOptiTree.hpp"
#include "qgmHintDef.hpp"
#include "qgmUtil.hpp"
#include "qgmConditionNode.hpp"

namespace engine
{
   class _qgmOptiUpdate : public _qgmOptiTreeNode
   {
   public:
      _qgmOptiUpdate( _qgmPtrTable *table, _qgmParamTable *param )
      :_qgmOptiTreeNode( QGM_OPTI_TYPE_UPDATE,
                         table, param ),
       _condition( NULL )
      {

      }

      virtual ~_qgmOptiUpdate()
      {
         SAFE_OSS_DELETE( _condition ) ;
      }

   public:
      virtual INT32 outputStream( qgmOpStream &stream )
      {
         return SDB_SYS ;
      }

      virtual string toString() const
      {
         return "update" ;
      }

      INT32 getHint( INT32 &flag ) const
      {
         INT32 rc = SDB_OK ;
         INT32 tmpFlag = 0 ;
         flag = 0 ;

         QGM_HINS::const_iterator itr = _hints.begin() ;
         for( ; itr != _hints.end(); ++itr )
         {
            if ( QGM_HINT_USEFLAG_SIZE == itr->value.size() &&
                 0 == ossStrncmp( itr->value.begin(), QGM_HINT_USEFLAG,
                                  itr->value.size() ) )
            {
               rc = qgmUseHintToFlag( *itr, tmpFlag ) ;
               if( rc )
               {
                  goto error ;
               }
               flag |= tmpFlag ;
            }
         }

      done :
         return rc ;
      error :
         goto done ;
      }

   private:
      virtual INT32 _checkPrivileges( ISession *session ) const;

   public:
      _qgmDbAttr           _collection ;
      BSONObj              _modifer ;
      _qgmConditionNode    *_condition ;
   } ;
   typedef class _qgmOptiUpdate qgmOptiUpdate ;
}

#endif


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

   Source File Name = qgmPlanContainer.hpp

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
#ifndef QGMPLANCONTAINER_HPP_
#define QGMPLANCONTAINER_HPP_

#include "qgmPlan.hpp"
#include "qgmParamTable.hpp"
#include "qgmPtrTable.hpp"

using namespace bson ;

namespace engine
{
   class _pmdEDUCB ;

   /*
      _qgmPlanContainer define
   */
   class _qgmPlanContainer : public SDBObject
   {
   public:
      _qgmPlanContainer() ;
      virtual ~_qgmPlanContainer() ;

   public:
      OSS_INLINE qgmParamTable *paramTable()
      {
         return &_paramT ;
      }

      OSS_INLINE qgmPtrTable *ptrTable()
      {
         return &_ptrT ;
      }

      OSS_INLINE qgmPlan *&plan()
      {
         return _plan ;
      }

      OSS_INLINE void close()
      {
         if ( NULL != _plan )
         {
            _plan->close() ;
         }
         return ;
      }

      OSS_INLINE QGM_PLAN_TYPE type()
      {
         return NULL == _plan ?
                QGM_PLAN_TYPE_MAX : _plan->type() ;
      }

      OSS_INLINE SQL_AST &ast()
      {
         return _ast ;
      }

      INT32 execute( _pmdEDUCB *cb ) ;
      BOOLEAN needRollback() const ;
      void    buildRetInfo( BSONObjBuilder &builder ) const ;

      INT32 fetch( BSONObj &obj ) ;

   private:
      SQL_AST _ast ;
      qgmParamTable _paramT ;
      qgmPtrTable _ptrT ;
      qgmPlan *_plan ;
   } ;

   typedef class _qgmPlanContainer qgmPlanContainer ;

   INT32 qgmDump ( _qgmPlanContainer *op, CHAR *pBuffer,
                   INT32 bufferSize ) ;
}

#endif


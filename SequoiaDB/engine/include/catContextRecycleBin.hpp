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

   Source File Name = catContextRecycleBin.hpp

   Descriptive Name = RunTime Context of Catalog Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context of Catalog.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#ifndef CAT_CONTEXT_RECYCLE_HPP_
#define CAT_CONTEXT_RECYCLE_HPP_

#include "catContextData.hpp"
#include "catRecycleBinManager.hpp"

namespace engine
{

   /*
      _catCtxReturnRecycleBin define
    */
   class _catCtxReturnRecycleBin : public _catCtxDataBase,
                                   public _catCtxRecycleHelper
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _catCtxReturnRecycleBin )
   public:
      _catCtxReturnRecycleBin( INT64 contextID, UINT64 eduID ) ;
      virtual ~_catCtxReturnRecycleBin() ;

      virtual const CHAR *name() const
      {
         return "CAT_RETURN_RECYCLEBIN" ;
      }

      virtual RTN_CONTEXT_TYPE getType() const
      {
         return RTN_CONTEXT_CAT_RETURN_RECYCLEBIN ;
      }

      INT32 open( const bson::BSONObj &queryObject,
                  BOOLEAN isReturnToName,
                  rtnContextBuf &buffObj,
                  _pmdEDUCB *cb ) ;

   protected:
      typedef _catCtxDataBase _BASE ;

      virtual INT32 _regEventHandlers() ;
      virtual INT32 _parseQuery( _pmdEDUCB *cb ) ;
      virtual INT32 _checkInternal( _pmdEDUCB *cb ) ;
      virtual INT32 _executeInternal( _pmdEDUCB *cb, INT16 w ) ;
      virtual INT32 _buildP1Reply( bson::BSONObjBuilder &builder ) ;

      virtual utilRecycleItem *_getRecycleItem()
      {
         return &_recycleItem ;
      }

      INT32 _checkReturnCS( utilRecycleItem &recycleItem,
                            _pmdEDUCB *cb ) ;
      INT32 _checkReturnCL( utilRecycleItem &recycleItem,
                            _pmdEDUCB *cb ) ;
      INT32 _checkReturnIdx( catReturnChecker &checker,
                             _pmdEDUCB *cb ) ;

   protected:
      // config to return recycle item
      catReturnConfig      _returnConf ;

      // info for conflict collections, collection spaces
      catRecycleReturnInfo _returnInfo ;

      // task handler
      catRtrnCtxTaskHandler _taskHandler ;
   } ;

   typedef class _catCtxReturnRecycleBin catCtxReturnRecycleBin ;

}

#endif //CATCONTEXTDATA_HPP_


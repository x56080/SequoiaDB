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

   Source File Name = clsRecycleBinJob.hpp

   Descriptive Name = Recycle Bin Job Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/01/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_RECYCLEBIN_JOB_HPP__
#define CLS_RECYCLEBIN_JOB_HPP__

#include "rtnBackgroundJobBase.hpp"
#include "utilLightJobBase.hpp"
#include "rtnLocalTaskFactory.hpp"
#include "monDMS.hpp"
#include "clsMgr.hpp"
#include "rtnCB.hpp"

using namespace std ;

namespace engine
{

   /*
      _clsDropRecycleBinItemJob define
    */
   class _clsDropRecycleBinJob : public _utilLightJob
   {
   public:
      _clsDropRecycleBinJob() ;
      virtual ~_clsDropRecycleBinJob() ;

      INT32 initDropItems( const UTIL_RECY_ITEM_LIST &recycleItems ) ;
      INT32 initDropAll() ;
      virtual INT32 doit( IExecutor *pExe,
                          UTIL_LJOB_DO_RESULT &result,
                          UINT64 &sleepTime ) ;

      virtual const CHAR *name() const
      {
         return _isDropAll ? "DropRecycleBinAll" : "DropRecycleBinItem" ;
      }

      INT32 dropItem( const utilRecycleItem &recycleItem,
                      _pmdEDUCB *cb,
                      BOOLEAN checkCatalog ) ;
      INT32 dropItems( const UTIL_RECY_ITEM_LIST &recycleItems,
                       _pmdEDUCB *cb,
                       BOOLEAN checkCatalog ) ;
      INT32 dropAll( _pmdEDUCB *cb, BOOLEAN checkCatalog ) ;

   protected:
      clsRecycleBinManager *  _recycleBinMgr ;
      BOOLEAN                 _isDropAll ;
      UTIL_RECY_ITEM_LIST     _recycleItems ;
   } ;

   typedef class _clsDropRecycleBinJob clsDropRecycleBinJob ;

   INT32 clsStartDropRecycleBinItemJob( const UTIL_RECY_ITEM_LIST &recycleItems ) ;
   INT32 clsStartDropRecycleBinAllJob() ;

}

#endif // CLS_RECYCLEBIN_JOB_HPP__

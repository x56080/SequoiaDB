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

   Source File Name = spdFMPMgr.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of Replication component. This file contains structure for
   replication control block.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPDFMPMGR_HPP_
#define SPDFMPMGR_HPP_

#include "ossLatch.hpp"
#include "sdbInterface.hpp"

#include <list>
#include <vector>

namespace engine
{
   class _pmdEDUCB ;
   class _spdFMP ;

   /*
      _spdFMPMgr define
   */
   class _spdFMPMgr : public _IControlBlock
   {
   public:
      _spdFMPMgr() ;
      virtual ~_spdFMPMgr() ;

      virtual SDB_CB_TYPE cbType() const { return SDB_CB_FMP ; }
      virtual const CHAR* cbName() const { return "FMPCB" ; }

      virtual INT32  init () ;
      virtual INT32  active () ;
      virtual INT32  deactive () ;
      virtual INT32  fini () ;

   public:
      INT32 getFMP( _spdFMP *&fmp ) ;
      INT32 returnFMP( _spdFMP *fmp, _pmdEDUCB *cb ) ;

   private:
      INT32 _createNewFMP( _spdFMP *&fmp ) ;

   private:
      std::list<_spdFMP *>       _pool ;
      std::vector< UINT32 >      _vecFreeSeqID ;
      UINT32                     _hwSeqID ;
      ossSpinXLatch              _mtx ;
      CHAR                       *_startBuf ;
      INT32                      _allocated ;

   } ;

   typedef class _spdFMPMgr spdFMPMgr ;

   /*
      get global fmp cb
   */
   spdFMPMgr* sdbGetFMPCB () ;

}

#endif


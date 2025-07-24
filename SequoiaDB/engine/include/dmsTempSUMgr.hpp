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

   Source File Name = dmsTempSUMgr.hpp

   Descriptive Name = DMS Temp Storage Unit Management Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS Temporary Storage Unit Management.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSTMPSUMGR_HPP__
#define DMSTMPSUMGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossLatch.hpp"
#include "dms.hpp"
#include "dmsSysSUMgr.hpp"
#include <queue>
#include <map>

using namespace std ;

namespace engine
{

   /*
      _dmsTempSUMgr define
   */
   class _dmsTempSUMgr : public _dmsSysSUMgr
   {
   private :
      // fifo queue, reserve operation always reserve the first one
      queue<UINT16>        _freeCollections ;
      // a reserved temp table will be stored in this map, with their EDU ID
      map<UINT16, UINT64>  _occupiedCollections ;

   public :
      _dmsTempSUMgr ( _SDB_DMSCB *dmsCB ) ;

      // this function verify whether SYSTEMP collection space exist. If it
      // is not exist then create one. And then reset all temp collections
      INT32 init() ;
      void  fini() ;

      INT32 release ( _dmsMBContext *&context ) ;

      INT32 reserve ( _dmsMBContext **ppContext, UINT64 eduID ) ;

   private:
      INT32       _cleanTmpPath() ;
      void        _removeTmpSu() ;
      INT32       _removeATmpFile( const CHAR *pPath,
                                   const CHAR *pPosix ) ;

   } ;
   typedef class _dmsTempSUMgr dmsTempSUMgr ;

}

#endif //DMSTMPSUMGR_HPP__


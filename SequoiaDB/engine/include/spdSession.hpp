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

   Source File Name = spdSession.hpp

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

#ifndef SPDSESSION_HPP_
#define SPDSESSION_HPP_

#include "core.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{
   class _spdFuncDownloader ;
   class _pmdEDUCB ;
   class _spdFMPMgr ;
   class _spdFMP ;

   class _spdSession : public SDBObject
   {
   public:
      _spdSession() ;
      virtual ~_spdSession() ;

   public:
      INT32 eval( const BSONObj &procedures,
                  _spdFuncDownloader *downloader,
                  _pmdEDUCB *cb ) ;

      INT32 next( BSONObj &obj ) ;

      const BSONObj &getErrMsg() { return _errmsg ; }
      const BSONObj &getRetMsg() { return _resmsg ; }
      INT32 resType() const { return _resType ; }
   private:
      INT32 _eval( const BSONObj &procedures,
                   _spdFuncDownloader *downloader ) ;

      INT32 _resIsOk( const BSONObj &res ) ;

   private:
      BSONObj _resmsg ;
      BSONObj _errmsg ;
      INT32 _resType ;
      _spdFMPMgr *_fmpMgr ;
      _pmdEDUCB *_cb ;
      _spdFMP *_fmp ;
   } ;

   typedef class _spdSession spdSession ;
}

#endif


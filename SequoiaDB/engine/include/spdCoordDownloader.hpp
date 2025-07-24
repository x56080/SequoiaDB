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

   Source File Name = spdCoordDownloader.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/19/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPDCOORDDOWNLOADER_HPP_
#define SPDCOORDDOWNLOADER_HPP_

#include "spdFuncDownloader.hpp"
#include "rtnContext.hpp"

namespace engine
{
   class _coordCommandBase ;
   class _SDB_RTNCB ;

   /*
      _spdCoordDownloader define
   */
   class _spdCoordDownloader : public _spdFuncDownloader
   {
   public:
      _spdCoordDownloader( _coordCommandBase *command,
                           _pmdEDUCB *cb ) ;
      virtual ~_spdCoordDownloader() ;

   public:
      virtual INT32 next( BSONObj &func ) ;
      virtual INT32 download( const BSONObj &matcher ) ;

   private:
      _rtnContextBuf _context ;
      SINT64 _contextID ;
      _coordCommandBase *_command ;
      _pmdEDUCB *_cb ;
      _SDB_RTNCB *_rtnCB ;
   } ;

   typedef class _spdCoordDownloader spdCoordDownloader ;
}

#endif // SPDCOORDDOWNLOADER_HPP_


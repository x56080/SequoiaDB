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

   Source File Name = dmsLobDirectOutBuffer.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_LOBDIRECTOUTBUFFER_HPP_
#define DMS_LOBDIRECTOUTBUFFER_HPP_

#include "dmsLobDirectBuffer.hpp"
#include "utilCache.hpp"

namespace engine
{
   /*
      _dmsLobDirectOutBuffer define
   */
   class _dmsLobDirectOutBuffer : public _dmsLobDirectBuffer 
   {
   public:
      _dmsLobDirectOutBuffer( const CHAR *usrBuf,
                              UINT32 size,
                              UINT32 offset,
                              BOOLEAN needAligned,
                              IExecutor *cb,
                              INT32 pageID,
                              UINT32 newestMask,
                              utilCachFileBase *pFile ) ;
      virtual ~_dmsLobDirectOutBuffer() ;

   public:
      virtual  INT32 doit( const tuple **pTuple ) ;
      virtual  void  done() ;

   private:
      utilCachFileBase     *_pFile ;
      INT32                _pageID ;
      UINT32               _newestMask ;

   } ;
   typedef class _dmsLobDirectOutBuffer dmsLobDirectOutBuffer ;
}

#endif //DMS_LOBDIRECTOUTBUFFER_HPP_


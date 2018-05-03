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

   Source File Name = dmsLobDirectInBuffer.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_LOBDIRECTINBUFFER_HPP_
#define DMS_LOBDIRECTINBUFFER_HPP_

#include "dmsLobDirectBuffer.hpp"

namespace engine
{
   /*
      _dmsLobDirectInBuffer define
   */
   class _dmsLobDirectInBuffer : public _dmsLobDirectBuffer
   {
   public:
      _dmsLobDirectInBuffer( CHAR *usrBuf,
                             UINT32 size,
                             UINT32 offset,
                             BOOLEAN needAligned,
                             IExecutor *cb ) ;
      virtual ~_dmsLobDirectInBuffer() ;

   public:
      virtual  INT32 doit( const tuple **pTuple ) ;
      virtual  void  done() ;

   private:

   } ;
   typedef class _dmsLobDirectInBuffer dmsLobDirectInBuffer ;
}

#endif //DMS_LOBDIRECTINBUFFER_HPP_


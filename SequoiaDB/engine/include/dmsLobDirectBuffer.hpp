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

   Source File Name = dmsLobDirectBuffer.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMS_LOBDIRECTBUFFER_HPP_
#define DMS_LOBDIRECTBUFFER_HPP_

#include "oss.hpp"
#include "core.hpp"
#include "sdbInterface.hpp"

namespace engine
{

   /*
      _dmsLobDirectBuffer define
   */
   class _dmsLobDirectBuffer : public SDBObject
   {
   public:
       typedef struct _tuple
       {
         CHAR    *buf ;
         UINT32   size ;
         UINT32   offset ;

         _tuple()
         {
            buf      = NULL ;
            size     = 0 ;
            offset   = 0 ;
         }
       } tuple ;

   public:
      _dmsLobDirectBuffer( CHAR *usrBuf,
                           UINT32 size,
                           UINT32 offset,
                           BOOLEAN needAligned,
                           IExecutor *cb ) ;

      virtual ~_dmsLobDirectBuffer() ;

      BOOLEAN  isAligned() const { return _aligned ; }

      INT32    prepare() ;

   public:
      virtual  INT32 doit( const tuple **pTuple ) = 0 ;
      virtual  void  done() = 0 ;

   protected:
      INT32 _extendBuf( UINT32 size ) ;

   protected:
      tuple       _t ;
      BOOLEAN     _aligned ;

      IExecutor   *_cb ;
      CHAR        *_buf ;
      UINT32      _bufSize ;

      CHAR        *_usrBuf ;
      UINT32      _usrSize ;
      UINT32      _usrOffset ;
   } ;
   typedef class _dmsLobDirectBuffer dmsLobDirectBuffer ;
}

#endif //DMS_LOBDIRECTBUFFER_HPP_


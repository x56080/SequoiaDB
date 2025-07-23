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

   Source File Name = dpsMessageBlock.hpp

   Descriptive Name = Data Management Service Temp Table Control Block

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   temporary table creation and release.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DPSMESSAGEBLOCK_H_
#define DPSMESSAGEBLOCK_H_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.h"
#include "pd.hpp"


namespace engine
{
   const UINT32 DPS_MSG_BLOCK_DEF_LEN = 1024 ;

   class _dpsMessageBlock : public SDBObject
   {
   private:
      CHAR *_start;
      CHAR *_write;
      CHAR *_read;
      // size is the size of buffer
      UINT32 _size;
      // length is the size with active user data
      UINT32 _length;

   public:
      _dpsMessageBlock( UINT32 size = DPS_MSG_BLOCK_DEF_LEN );
      _dpsMessageBlock( const _dpsMessageBlock &mb ) ;
      _dpsMessageBlock &operator=(const _dpsMessageBlock &mb ) ;
      ~_dpsMessageBlock();
   public:
      OSS_INLINE UINT32 size() const
      {
         return _size;
      }

      OSS_INLINE UINT32 idleSize() const
      {
         return _size - _length;
      }

      OSS_INLINE UINT32 length() const
      {
         return _length;
      }

      OSS_INLINE CHAR *writePtr() const
      {
         return _write;
      }

      OSS_INLINE void writePtr( UINT32 offset )
      {
         _write = _start + offset;
         _length = offset;
         SDB_ASSERT( _length <= _size , "out of mem!" ) ;
         return;
      }

      OSS_INLINE void invalidateData()
      {
         ossMemset( _write, 0, idleSize() ) ;
      }

      OSS_INLINE const CHAR *readPtr() const
      {
         return _read;
      }

      OSS_INLINE void readPtr( INT32 offset )
      {
         _read = _start + offset;
      }

      OSS_INLINE CHAR *offset( UINT32 offset )const
      {
         return _start + offset;
      }

      OSS_INLINE void clear()
      {
         _write = _start;
         _read = _start;
         _length = 0;
      }

      OSS_INLINE const CHAR *startPtr() const
      {
         return _start ;
      }

      INT32 extend( UINT32 len );
   };
   typedef class _dpsMessageBlock dpsMessageBlock;
}

#endif

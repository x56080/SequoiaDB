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

   Source File Name = dmsTmpBlkUnit.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for IO operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef DMSTMPBLKUNIT_HPP_
#define DMSTMPBLKUNIT_HPP_

#include "ossIO.hpp"
#include <string>
#include <list>

#define DMS_TMP_BLK_FILE_BEGIN "TMP_"

namespace engine
{
   class _dmsTmpBlkUnit ;

   class _dmsTmpBlk
   {
   public:
      _dmsTmpBlk() ;
      ~_dmsTmpBlk() ;

   public:
      void reset()
      {
         _read = 0 ;
      }

      std::string toString()const ;

      const UINT64 &size() const
      {
         return _size ;
      }

   private:
      UINT64 _begin ;
      UINT64 _size ;
      UINT64 _read ;

      friend class _dmsTmpBlkUnit ;
   } ;
   typedef class _dmsTmpBlk dmsTmpBlk ;

   typedef std::list<_dmsTmpBlk> RTN_SORT_BLKS ;
   typedef SINT64 DMS_TMP_FILE_ID ;

   class _dmsTmpBlkUnit
   {
   public:
      _dmsTmpBlkUnit() ;
      virtual ~_dmsTmpBlkUnit() ;

   public:
      OSS_INLINE UINT64 totalSize() const
      {
         return _totalSize ;
      }

      OSS_INLINE BOOLEAN isOpened() const
      {
         return _opened ;
      }

      INT32 openFile( const CHAR *path,
                      const DMS_TMP_FILE_ID &id ) ;

      INT32 write( const void *buf,
                   UINT64 size,
                   BOOLEAN extendSize ) ;

      INT32 seek( const UINT64 &offset ) ;

      INT32 read( dmsTmpBlk &blk, UINT64 size,
                  void *buf, UINT64 &got ) ;

      /// blks should be managed by user.
      /// especially rebuild a blk on a old blk.
      INT32 buildBlk( const UINT64 &begin,
                      const UINT64 &size,
                      dmsTmpBlk &blk ) ;
   private:
      INT32 _removeFile() ;

   private:
      std::string _fullPath ;
      _OSS_FILE _file ;
      UINT64 _totalSize ;
      BOOLEAN _opened ;
   } ;
}

#endif


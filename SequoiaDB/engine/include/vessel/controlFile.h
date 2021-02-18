/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = controlFile.h

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_CONTROL_FILE_H_
#define VESSEL_CONTROL_FILE_H_

#include "ossFile.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   const UINT32 VESSEL_CONTROL_FILE_SIZE = 512;
   enum VESSEL_CF_STATUS
   {
      VESSEL_CF_STATUS_NORMAL = 0,
      VESSEL_CF_STATUS_UNUSED = 1,
      VESSEL_CF_STATUS_ABNORMAL = 2,
   };

   class controlFile : public SDBObject
   {
      public:
         controlFile();
         virtual ~controlFile();

      public:
         struct head
         {
            OSS_INLINE head()
            :version(0),
             userType(65535),
             contentLen(0),
             pad(0),
             sequence(0),
             updateTime(0){}

            UINT16 version;
            UINT16 userType;
            UINT16 contentLen;
            UINT16 pad;
            UINT64 sequence;
            UINT64 updateTime; /// milli seconds

            BOOLEAN valid()const
            {
               return 0 < contentLen && 65535 != userType;
            }
         }; // struct head

      public:
         INT32 open(const CHAR *path);
         INT32 close();

         OSS_INLINE BOOLEAN isOpen()const
         {
            return NULL != _sequences;
         }

         OSS_INLINE UINT32 getMaxVersionCount()const
         {
            return _count;
         }

         /// create a new control file and fsync
         INT32 commit(UINT32 size, const void *buf);

         INT32 readLatestVersion(head *head, void *buf)const;

         INT32 readOldestVersion(head *head, void *buf)const;

         /// for unit tests
         INT32 read(UINT64 sequence, head *head, void *buf)const;
      public:
         /// return file name prefix. final file name: prefix.control.<num>
         virtual const CHAR *getName()const = 0;
         /// return sequence window
         virtual UINT32 getSeqWindow()const
         {
            return 8;
         }

         /// do some validations when open
         ///virtual INT32 onOpen(UINT32 versionCnt) = 0;

         virtual UINT16 getUserType()const = 0;

         virtual std::string toString(const CHAR *body)const = 0;

      protected:
         INT32 write(UINT32 size, const void *buf);

      private:
         INT32 initMem(UINT32 count);
         BOOLEAN getNextPosition(UINT32 &p)const;
         
         OSS_INLINE head *getHead(UINT32 pos)
         {
            if (OSS_UNLIKELY(_count <= pos))
            {
               return NULL;
            }
            return (head *)(_buf + pos * VESSEL_CONTROL_FILE_SIZE);
         }
         OSS_INLINE const head *getHead(UINT32 pos) const
         {
            if (OSS_UNLIKELY(_count <= pos))
            {
               return NULL;
            }
            return (head *)(_buf + pos * VESSEL_CONTROL_FILE_SIZE);
         }
         OSS_INLINE CHAR *getBuf(UINT32 pos)
         {
            if (OSS_UNLIKELY(_count < pos))
            {
               return NULL;
            }
            return (_buf + pos * VESSEL_CONTROL_FILE_SIZE); 
         }
         OSS_INLINE const CHAR *getBuf(UINT32 pos)const
         {
            if (OSS_UNLIKELY(_count < pos))
            {
               return NULL;
            }
            return (_buf + pos * VESSEL_CONTROL_FILE_SIZE); 
         }
         OSS_INLINE ossFile *getFile(UINT32 pos)
         {
            if (OSS_UNLIKELY(_count < pos))
            {
               return NULL;
            }
            return &(_files[pos]);
         }
      private:
         UINT32 _count;
         INT32 _maxSeqSlot;
         UINT64 *_sequences;
         CHAR *_buf;
         ossFile *_files;
   }; // class controlFile
}// namespace vessel
}// namespace engine

#endif //VESSEL_CONTROL_FILE_H_
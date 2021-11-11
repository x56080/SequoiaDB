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
#include "ossUtil.h"
#include "ossMemPool.hpp"
#include "strSlice.h"
#include "vessel/vesselFileName.h"

namespace engine
{
namespace vessel
{
   static const UINT32 INVALID_CONTROL_FILE_VERSION = 0;
   static const UINT32 CONTROL_FILE_VERSION = 1;
   static const UINT32 CONTROL_FILE_SIZE = 4096;

   static const UINT64 INVALID_COMMIT_VERSION = OSS_UINT64_MAX;

   class controlFile : public SDBObject
   {
      public:
         controlFile();
         virtual ~controlFile();

         controlFile(const controlFile &) = delete;
         controlFile &operator=(const controlFile &) = delete;

      public:
#pragma pack(4)
         struct head
         {
            OSS_INLINE head(){}
            OSS_INLINE ~head(){}

            UINT32 magicCode = 0;
            UINT32 checksum = 0;
            UINT32 headVerion = INVALID_CONTROL_FILE_VERSION;
            UINT32 flags = 0;
            UINT64 commitVersion = INVALID_COMMIT_VERSION;
            UINT64 updateMillis = 0; /// milli seconds
            UINT32 contentLen = 0; 
            UINT64 pad[2] = {0,0};

            OSS_INLINE head &operator=(const head &h)
            {
               magicCode = h.magicCode;
               checksum = h.checksum;
               headVerion = h.headVerion;
               flags = h.flags;
               commitVersion = h.commitVersion;
               updateMillis = h.updateMillis;
               contentLen = h.contentLen;
               ossMemcpy(pad, h.pad, sizeof(pad));
               return *this;
            }

            OSS_INLINE BOOLEAN isUnused()const
            {
               return INVALID_COMMIT_VERSION == commitVersion;
            }
         }; // struct head
#pragma pack()

      public:
         //INT32 create(const strSlice &dir);
         INT32 open(const strSlice &dir);
         void close();
         void destroy();

         OSS_INLINE BOOLEAN isOpen()const
         {
            return !_dir.empty();
         }
         OSS_INLINE UINT32 getAliveVersionCount()const
         {
            return _workshop.size();
         }
         OSS_INLINE UINT32 getTotalFileCount()const
         {
            return _workshop.size() + _unused.size();
         }

         /// create a new version.
         INT32 commit(UINT32 size, const void *buf);

         INT32 readLatestVersion(UINT64 &version,
                                 UINT32 bufSize,
                                 void *buf)const;

         /// if 0 == preCountOfLatest, return latest version.
         INT32 readPreVersion(UINT32 preCountOfLatest,
                              UINT64 &version,
                              UINT32 bufSize,
                              void *buf)const;

         INT32 readOldestVersion(UINT64 &version,
                                 UINT32 bufSize,
                                 void *buf)const;
      public:
         /// return control file's name.
         /// "i" is file sequence.
         virtual BOOLEAN getFileName(UINT32 i,
                                     std::string &name)const = 0;

         /// return max alive version count. valid range is(0, 64];
         /// which defines the max count of files.
         virtual UINT32 getMaxAliveVersionCount()const = 0;

      private:
         struct _fileObj : public SDBObject
         {
            _fileObj()
            {}
            ~_fileObj()
            {
               if (file.isOpened())
               {
                  ossClose(file);
               }
            }

            _fileObj(const _fileObj &) = delete;
            _fileObj &operator=(const _fileObj &) = delete;

            OSS_INLINE const head *getHead()const
            {
               return (const head *)buf;
            }
            OSS_INLINE head *getHead()
            {
               return (head *)buf;
            }

            OSS_INLINE void close()
            {
               sequence = 0;
               name.clear();
               ossMemset(buf, 0, sizeof(buf));
               ossClose(file);
               return;
            }

            UINT32 sequence = 0;
            std::string name;
            CHAR buf[CONTROL_FILE_SIZE] = {};
            _OSS_FILE file;
         };//struct _fileObj

         typedef ossPoolList<_fileObj *> _FILE_OBJ_LIST;

      private:
         //INT32 createFiles();
         INT32 openFiles();

         void initFileBuf(_fileObj *obj);
         void updateFileBuf(_fileObj *obj, UINT32 size, const void *buf);
         void pushToUnusedListWhenOpen(_fileObj *obj);
         void pushToWorkshopWhenOpen(_fileObj *obj);

         INT32 commitFromUnusedList(UINT32 size, const void *buf);
         INT32 commitFromWorkshop(UINT32 size, const void *buf);
         INT32 writeFile(_fileObj *obj);
         INT32 readFile(_fileObj *obj);
         INT32 read(const _fileObj *obj,
                    UINT32 size,
                    head &h,
                    void *buf)const;

      private:
         std::string _dir;
         UINT64 _commitVersion = 0;/// next commit version.
         _FILE_OBJ_LIST _workshop;
         _FILE_OBJ_LIST _unused;
   }; // class controlFile
}// namespace vessel
}// namespace engine

#endif //VESSEL_CONTROL_FILE_H_
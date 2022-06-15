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
#include "strSlice.h"
#include "vessel/invalidFileReason.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 CONTROL_FILE_HEAD_VERSION = 1;
   constexpr UINT32 MAX_CONTROL_FILE_LEN = 16 * 1024 * 1024;

   class controlFile : public SDBObject
   {
      public:
         controlFile(){}
         ~controlFile();
         controlFile(const controlFile &) = delete;
         controlFile &operator=(const controlFile &) = delete;

      public:
#pragma pack(4)
         struct controlFileHead
         {
            controlFileHead &operator=(const controlFileHead &h)
            {
               magicCode = h.magicCode;
               checksum = h.checksum;
               headVersion = h.headVersion;
               contentLen = h.contentLen;
               creationTime = h.creationTime;
               return *this;
            }

            UINT32 magicCode = 0;
            UINT32 checksum = 0;
            UINT32 headVersion = 0;
            UINT32 contentLen = 0;
            UINT64 creationTime = 0;
         };
#pragma pack()

      public:
         static INT32 create(const strSlice &fullPath,
                             const CHAR *buf,
                             UINT32 bufSize,
                             BOOLEAN replace = FALSE,
                             BOOLEAN chmod = TRUE);

         void close();

         /// return SDB_VESSEL_INVALID_CONTROL_FILE if failed to validate file content.
         /// any other else codes mean hard error.
         INT32 openToRead(const strSlice &fullPath,
                          invalidFileReason &reason);

         INT32 read(CHAR *buf, UINT32 size);

      public:
         UINT32 getContentLen()const;

         UINT64 getCreationTime()const;

      private:
         INT32 validateFile(const CHAR *buf, UINT32 size, invalidFileReason &reason);

      private:
         OSSFILE _file;
         controlFileHead _head;
   }; // class controlFile
}// namespace vessel
}// namespace engine

#endif //VESSEL_CONTROL_FILE_H_
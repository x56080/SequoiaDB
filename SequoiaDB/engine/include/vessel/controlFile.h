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

   Source File Name = controlFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
                             BOOLEAN chmod = TRUE,
                             UINT32* contentLen = nullptr,
                             UINT64* creationTime = nullptr);

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
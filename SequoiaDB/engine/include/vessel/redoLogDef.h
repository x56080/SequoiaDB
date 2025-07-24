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

   Source File Name = redoLogDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_REDO_LOG_DEF_H_
#define VESSEL_REDO_LOG_DEF_H_

#include "dpsDef.hpp"

#include <type_traits>

namespace engine
{
namespace vessel
{
   constexpr UINT32 RLOG_BUFFER_PAGE_SIZE = 64 << 10;
   constexpr UINT32 RLOG_FILE_BODY_SIZE = 64 << 20;
   static_assert(0 == RLOG_FILE_BODY_SIZE % RLOG_BUFFER_PAGE_SIZE, "must be aligned");

#pragma pack(4)
   struct redoLogFileHeader
   {
      void init(UINT32 fileSize, UINT32 logicalId, UINT64 startLSN);
      BOOLEAN validate() const;

      static constexpr CHAR *EYE_CATCHER_STR = "SDBVREDO";
      static constexpr UINT32 EYE_CACHER_LEN = 8;
      static constexpr UINT32 CURRENT_VERSION = 1;

      CHAR eyeCatcher[EYE_CACHER_LEN] = {};
      UINT32 version = 0;
      UINT32 fileSize = 0;
      UINT32 flags = 0;
      UINT32 logicalId = 0;
      UINT64 startLSN = DPS_INVALID_LSN_OFFSET;
      CHAR pad[65504] = {};
   };
#pragma pack()

   static_assert(std::is_standard_layout<redoLogFileHeader>::value, "must be standard layout");
   constexpr UINT32 RLOG_FILE_HEAD_SIZE = sizeof(redoLogFileHeader);
   static_assert(65536 == RLOG_FILE_HEAD_SIZE, "must be 64KB");
   constexpr UINT32 RLOG_FILE_SIZE = RLOG_FILE_BODY_SIZE + RLOG_FILE_HEAD_SIZE;

   constexpr CHAR *RLOG_FILE_NAME_PREFIX = "sdbvRedo";

   struct redoLogOptions : public SDBObject
   {
      UINT32 totalBufSize = 64 << 20;
   };

   struct redoLogFilesSummary : public SDBObject
   {
      UINT32 totalFiles = 0;
      UINT32 readonlyFiles = 0;
      UINT64 oldestLSN = DPS_INVALID_LSN_OFFSET;
      UINT64 currentLSN = DPS_INVALID_LSN_OFFSET;
   };
} // namespace vessel

} // namespace engine


#endif//VESSEL_REDO_LOG_DEF_H_
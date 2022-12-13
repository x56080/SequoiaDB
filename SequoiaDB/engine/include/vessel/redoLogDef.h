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

   Source File Name = redoLogDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
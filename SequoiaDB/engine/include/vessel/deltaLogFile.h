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

   Source File Name = deltaLogFile.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DELTA_LOG_FILE_H_
#define VESSEL_DELTA_LOG_FILE_H_

#include "vessel/extentStorageFile.h"

namespace engine
{
namespace vessel
{
   static const UINT32 DELTA_LOG_FILE_PAGESIZE = 4096;
   static const UINT32 DELTA_LOG_FILE_PAGE_COUNT_IN_SEG = 256;
   static const UINT32 DELTA_LOG_FILE_MAX_SEG = 32;
   constexpr UINT32 DELTA_LOG_FILE_MAX_PAGE_COUNT = DELTA_LOG_FILE_PAGE_COUNT_IN_SEG *
                                                    DELTA_LOG_FILE_MAX_SEG;
   constexpr UINT32 DELTA_LOG_FILE_MAX_OFFSET = DELTA_LOG_FILE_MAX_PAGE_COUNT *
                                                DELTA_LOG_FILE_PAGESIZE;

   class deltaLogFile : public extentStorageFile
   {
      public:
         deltaLogFile(){}
         ~deltaLogFile(){}

      private:
         virtual FILE_TYPE getFileType()const
         {
            return FILE_TYPE_DELTA;
         }
         virtual const CHAR *getMagicChars()const
         {
            return "SDBVDLTF";
         }
   };//class deltaLogFile
}//namespace vessel
}//namespace engine

#endif//VESSEL_DELTA_LOG_FILE_H_
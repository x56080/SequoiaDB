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

   Source File Name = compressionDef.h

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

#ifndef VESSEL_COMPRESSION_DEF_H_
#define VESSEL_COMPRESSION_DEF_H_

namespace engine
{
namespace vessel
{
   enum CL_COMPRESSION_TYPE
   {
      CL_COMPRESSION_TYPE_NONE = 0,
      CL_COMPRESSION_TYPE_GLOBAL_DIC = 1,
      CL_COMPRESSION_TYPE_PAGE_DIC = 2, 
      CL_COMPRESSION_TYPE_MAX = CL_COMPRESSION_TYPE_PAGE_DIC,
   };

   enum CL_COMPRESSION_ALGRITHM
   {
      CL_COMPRESSION_ALGRITHM_LZW = 0,
      CL_COMPRESSION_ALGRITHM_LZ4 = 1,
      CL_COMPRESSION_ALGRITHM_MAX = CL_COMPRESSION_ALGRITHM_LZ4,
   };

   enum CL_COMPRESSION_LEVEL
   {
      CL_COMPRESSION_LEVEL_MIN = 0,
      CL_COMPRESSION_LEVEL_MAX = 9,
   };
}//namespace vessel
}//namespace engine

#endif//VESSEL_COMPRESSION_DEF_H_
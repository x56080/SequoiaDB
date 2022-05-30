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

   Source File Name = lsmDBDef.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_DB_DEF_H_
#define VESSEL_LSM_DB_DEF_H_

#include "oss.hpp"
namespace engine
{
namespace vessel
{
   enum LSM_CF_TYPE
   {
      LSM_DEFAULT_CF = 0,
      LSM_INDEX_CF = 1,
      LSM_LOB_CHUNK_CF = 2,
      LSM_MAX_CF_TYPE = LSM_LOB_CHUNK_CF
   };

   // The first column family name must be 'default'.
   constexpr CHAR *LSM_DEFAULT_CF_NAME = "default";
   constexpr CHAR *LSM_INDEX_CF_NAME = "sdb.lsmIndex";
   constexpr CHAR *LSM_LOB_CHUNK_CF_NAME = "sdb.lsmLobChunk";
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_DB_DEF_H_
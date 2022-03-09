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

   Source File Name = runtimeMbContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/runtimeMbContext.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   runtimeMbContext::~runtimeMbContext()
   {
      SDB_ASSERT(_rlc.isEmpty(), "unlocking missed");
   }

   void runtimeMbContext::init(const clMetaBlock &cmb,
                               const collectionSpaceId &csIdentifer)
   {
      SDB_ASSERT(cmb.isValid(), "can not be invalid");
      SDB_ASSERT(csIdentifer.isValid(), "can not be invalid");
      SDB_ASSERT(_rlc.isEmpty(), "do not reinit");
      _gcid.reset(csIdentifer,
                  collectionId(cmb.logicalCLID, cmb.innerID, cmb.mbID));
      _name.reset(cmb.name);
      SDB_ASSERT(cmb.minFreePercent <= 100, "out of range");
      _minFreePercent = cmb.minFreePercent;
      _compressionType = (UTIL_COMPRESSOR_TYPE)(cmb.compressionType);
   }

   void runtimeMbContext::fini()
   {
      _gcid.reset();
      _name.reset();
      _minFreePercent = 0.0f;
      _compressionType = UTIL_COMPRESSOR_INVALID;
      _rlc.fini();
   }
} // namespace vessel

} // namespace engine

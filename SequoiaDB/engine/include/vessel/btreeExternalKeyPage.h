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

   Source File Name = btreeExternalKeyPage.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_EXTERNAL_KEY_PAGE_H_
#define VESSEL_BTREE_EXTERNAL_KEY_PAGE_H_

#include "vessel/pageDef.h"

namespace engine
{
namespace vessel
{
   static const UINT32 BTREE_EXT_KEY_PAGE_HEAD_VERSION = 1;
#pragma pack(4)
   struct btreeExternalKeyPageHead
   {
      UINT32 version = 0;
      UINT32 flags = 0;
      UINT32 indexId = 0;
      UINT32 size = 0;

   };//struct btreeExternalKeyPageHead
#pragma pack()

   static const UINT32 BTREE_EXT_KEY_PAGE_HEAD_SIZE = sizeof(btreeExternalKeyPageHead);

   BOOLEAN initBtreeExtKeyPage(UINT32 pageSize,
                               PAGE_ID pid,
                               PAGE_ID lpid,
                               PAGE_SNAPSHOT_VERION psv,
                               UINT32 indexId,
                               UINT32 keySize,
                               const CHAR *keyData,
                               CHAR *buf);
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_EXTERNAL_KEY_PAGE_H_
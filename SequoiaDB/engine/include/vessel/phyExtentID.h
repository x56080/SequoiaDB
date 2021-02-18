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

   Source File Name = phyExtentID.hpp

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

#ifndef VESSEL_PHY_EXTENT_ID_H_
#define VESSEL_PHY_EXTENT_ID_H_

#include "vessel/vesselDef.h"
#include "vessel/extentDef.h"
#include <sstream>
//#define XXH_INLINE_ALL
//#include "xxhash/xxhash.h"

namespace engine
{
namespace vessel
{
class physicalExtentID
{
   public:
      OSS_INLINE physicalExtentID()
      :_space(INVALID_SPACE_ID),
       _type(INVALID_SPACE_TYPE),
       _pad(0),
       _page(INVALID_PAGE_ID)
      {
         /// do nothing
      }

      OSS_INLINE physicalExtentID(SPACE_ID sid, SPACE_TYPE type, PAGE_ID pid)
      :_space(sid), _type(type), _pad(0), _page(pid)
      {
      
      }

      OSS_INLINE physicalExtentID(const physicalExtentID &r)
      :_space(r._space), _type(r._type), _pad(0), _page(r._page)
      {
      
      }

      OSS_INLINE physicalExtentID &operator=(const physicalExtentID &r)
      {
         _space = r._space;
         _type = r._type;
         _page = r._page;
         return *this;
      }

      OSS_INLINE BOOLEAN invalid()const
      {
         return INVALID_SPACE_ID == _space ||
                INVALID_SPACE_TYPE == _type ||
                INVALID_PAGE_ID == _page;
      }

      OSS_INLINE void reset(SPACE_ID sid, SPACE_TYPE type, PAGE_ID pid)
      {
         _space = sid;
         _type = type;
         _page = pid;
         return;
      }

      OSS_INLINE void reset()
      {
         _space = INVALID_SPACE_ID;
         _type = INVALID_SPACE_TYPE;
         _page = INVALID_PAGE_ID;
         return;
      }

      OSS_INLINE UINT32 hash()const
      {
         //return XXH3_64bits(this, sizeof(physicalExtentID));
         UINT32 hash = _space;
         hash = hash << 16;
         hash += _space + _type + _page;
         return hash;
      }

      OSS_INLINE BOOLEAN operator==(const physicalExtentID &r)const
      {
         return _space == r._space && _type == r._type && _page == r._page;
      }

      OSS_INLINE BOOLEAN operator<(const physicalExtentID &r) const
      {
         if (_space < r._space)
         {
            return TRUE;
         }
         else if (_space > r._space)
         {
            return FALSE;
         }
         else if (_type < r._type)
         {
            return TRUE;
         }
         else if (_type > r._type)
         {
            return FALSE;
         }
         else
         {
            return _page < r._page;
         }
      }

      OSS_INLINE SPACE_ID space() const
      {
         return _space;
      }

      OSS_INLINE SPACE_TYPE type() const
      {
         return _type;
      }

      OSS_INLINE PAGE_ID page()const
      {
         return _page;
      }

      std::string toString()const
      {
         std::stringstream ss;
         ss << "{SPACE_ID:" << _space
            << ",TYPE:" << _type
            << ",PAGE_ID:" << _page
            << "}";
         return ss.str();
      }

   public:
      SPACE_ID _space;
      SPACE_TYPE _type;
      UINT8 _pad;
      PAGE_ID _page;
}; /// end of physicalExtentID


typedef physicalExtentID PHY_EXTENT_ID;
typedef physicalExtentID GLOBAL_PAGE_ID;

class fullPageID
{
   public:
      OSS_INLINE fullPageID():
      lpid(INVALID_PAGE_ID)
      {}
      OSS_INLINE ~fullPageID()
      {}

   public:
      physicalExtentID gpid;
      PAGE_ID lpid;
};//class fullPageID

typedef fullPageID GLOBAL_FULL_PAGE_ID;

} /// end of namespace vessel
} /// end of namespace engine

#endif

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

   Source File Name = indexKeyPattern.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_KEY_PATTERN_H_
#define VESSEL_INDEX_KEY_PATTERN_H_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/bson.hpp"
#include "vessel/orderingWrapper.h"

namespace engine
{
namespace vessel
{
   class indexKeyPattern : public SDBObject
   {
      public:
         indexKeyPattern();
         indexKeyPattern(const indexKeyPattern &o):
         _keyCount(o._keyCount),
         _ordering(o._ordering),
         _pattern(o._pattern.getOwned())
         {}

         ~indexKeyPattern();
         indexKeyPattern &operator=(const indexKeyPattern &o)
         {
            _keyCount = o._keyCount;
            _ordering = o._ordering;
            _pattern = o._pattern.getOwned();
            return *this;
         }

         BOOLEAN operator==(const indexKeyPattern &o)const;
      
      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return 0 < _keyCount;
         }
         OSS_INLINE UINT32 getKeyCount()const
         {
            return _keyCount;
         }

         orderingWrapper getOrdering()const;

         OSS_INLINE const bson::BSONObj &getPattern()const
         {
            return _pattern;
         }
   
      public:
         INT32 set(const bson::BSONObj &obj);
         void reset();
         BOOLEAN isCoveredBy(const indexKeyPattern &other)const;

      private:
         UINT32 _keyCount = 0;
         UINT32 _ordering = 0;
         bson::BSONObj _pattern;
   };//class indexKeyPattern
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_KEY_PATTERN_H_
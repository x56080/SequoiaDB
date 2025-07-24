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

   Source File Name = indexKeyPattern.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
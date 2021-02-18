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

   Source File Name = lcExtentTagHolder.h

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

#ifndef VESSEL_LC_EXTENT_TAG_HOLDER_H_
#define VESSEL_LC_EXTENT_TAG_HOLDER_H_

#include "vessel/lcExtentTag.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   class lcExtentTagHolder
   {
      public:
         OSS_INLINE lcExtentTagHolder():_tag(NULL), _lockMode(LOCK_MODE_NONE){}
         OSS_INLINE ~lcExtentTagHolder()
         {
            /// WARNING: holder's destructor will not release lock automaticly, which
            /// to force users to manage lock resources carefully
            _tag = NULL;
            _lockMode = LOCK_MODE_NONE;
         }

         OSS_INLINE lcExtentTagHolder(const lcExtentTagHolder &r)
         :_tag(r._tag), _lockMode(r._lockMode)
         {}
         
         OSS_INLINE lcExtentTagHolder &operator=(const lcExtentTagHolder &r)
         {
            _tag = r._tag;
            _lockMode = r._lockMode;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN valid()const
         {
            return NULL != _tag;
         }

         OSS_INLINE void reset(lcExtentTag *tag)
         {
            _tag = tag;
            _lockMode = LOCK_MODE_NONE;
         }

         OSS_INLINE void unlock()
         {
            if (OSS_LIKELY(NULL != _tag))
            {
               if (LOCK_MODE_SHARED == _lockMode)
               {
                  _tag->rwMutex().unlock_shared();
               }
               else if (LOCK_MODE_UPGRADE == _lockMode)
               {
                  _tag->rwMutex().unlock_upgrade();
               }
               else if (LOCK_MODE_UNIQUE == _lockMode)
               {
                  _tag->rwMutex().unlock();
               }

               _lockMode = LOCK_MODE_NONE;
            }
            return;
         }

         OSS_INLINE void lockUnique()
         {
            _tag->rwMutex().lock();
            _lockMode = LOCK_MODE_UNIQUE;
         }

         OSS_INLINE BOOLEAN tryLockUnique()
         {
            if (_tag->rwMutex().try_lock())
            {
               _lockMode = LOCK_MODE_UNIQUE;
               return TRUE;
            }
            return FALSE;
         }

         OSS_INLINE void unlockUnique()
         {
            _tag->rwMutex().unlock();
            _lockMode = LOCK_MODE_NONE;
         }

         OSS_INLINE void lockShared()
         {
            _tag->rwMutex().lock_shared();
            _lockMode = LOCK_MODE_SHARED;
         }

         OSS_INLINE void unlockShared()
         {
            _tag->rwMutex().unlock_shared();
            _lockMode = LOCK_MODE_NONE;
         }

         OSS_INLINE void lockUpgrade()
         {
            _tag->rwMutex().lock_upgrade();
            _lockMode = LOCK_MODE_UPGRADE;
         }

         OSS_INLINE void lockUniqueFromUpgrade()
         {
            if (LOCK_MODE_UPGRADE == _lockMode)
            {
               _tag->rwMutex().unlock_upgrade_and_lock();
               _lockMode = LOCK_MODE_UNIQUE;
            }
         }

         OSS_INLINE void unlockUniqueAndlockUpgrade()
         {
            if (LOCK_MODE_UNIQUE == _lockMode)
            {
               _tag->rwMutex().unlock_and_lock_upgrade();
               _lockMode = LOCK_MODE_UPGRADE;
            }
         }

         OSS_INLINE void lockWithMode(LOCK_MODE mode)
         {
            if (LOCK_MODE_SHARED == mode)
            {
               _tag->rwMutex().lock_shared();
               _lockMode = mode;
            }
            else if (LOCK_MODE_UPGRADE == mode)
            {
                _tag->rwMutex().lock_upgrade();
                _lockMode = mode;
            }
            else if (LOCK_MODE_UNIQUE == mode)
            {
               _tag->rwMutex().lock();
               _lockMode = mode;
            }
         }

         OSS_INLINE LOCK_MODE getLockMode()const
         {
            return _lockMode;
         }

         OSS_INLINE lcExtentTag *tag()
         {
            return _tag;
         }

         OSS_INLINE const lcExtentTag *tag() const
         {
            return _tag;
         }
      private:
         lcExtentTag *_tag;
         enum LOCK_MODE _lockMode;
   }; /// end of class lcExtentTagHolder
} /// end of namespace vessel
} /// end of namespace engine

#endif /// end of VESSEL_EXTENT_TAG_HOLDER_H_
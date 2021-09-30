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

   Source File Name = btreeAccessContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BTREE_ACCESS_CONTEXT_H_
#define VESSEL_BTREE_ACCESS_CONTEXT_H_

#include "vessel/btreeNodePath.h"
#include "vessel/recordID.h"
#include "ixmKey.hpp"

namespace engine
{
namespace vessel
{
   class btreeAccessContext : public SDBObject
   {
      public:
         btreeAccessContext() = delete;
         btreeAccessContext(const indexContext *ic);
         virtual ~btreeAccessContext(){}
         btreeAccessContext(const btreeAccessContext &) = delete;
         btreeAccessContext &operator=(const btreeAccessContext &) = delete;


      public:
         OSS_INLINE const ixmKey &getKey()const
         {
            return _key;
         }
         OSS_INLINE BOOLEAN isObstructed()const
         {
            return _obstructed;
         }
         OSS_INLINE void setObstructed()
         {
            _obstructed = TRUE;
         }
         OSS_INLINE btreeNodePath &getPath()
         {
            return _path;
         }

      protected:
         ixmKey _key;
         btreeNodePath _path;
         BOOLEAN _obstructed = FALSE;
   };//class btreeAccessContext

   class btreeInsertContext : public btreeAccessContext
   {
      public:
         btreeInsertContext(const indexContext *ic);
         virtual ~btreeInsertContext();

      public:
         OSS_INLINE const recordID &getRid()const
         {
            return _rid;
         }
         OSS_INLINE const DPS_TRANS_ID &getTransID()const
         {
            return _transID;
         }
         OSS_INLINE BOOLEAN isPessimistically()const
         {
            return _pessimistically;
         }
         OSS_INLINE void setPessimistically()
         {
            _pessimistically = TRUE;
         }
      
      public:
         INT32 init(const ixmKey &key,
                    const recordID &rid,
                    const DPS_TRANS_ID &transID);

         void fini();

      private:
         recordID _rid;
         DPS_TRANS_ID _transID;
         BOOLEAN _pessimistically = FALSE;
         RECORD_SLOT_ID _pos = INVALID_RECORD_SLOT_ID;
   };//class btreeInsertContext
} // namespace vessel

} // namespace engine


#endif//VESSEL_BTREE_ACCESS_CONTEXT_H_
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

   Source File Name = atomicOperationList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_ATOMIC_OPERATOION_LIST_H_
#define VESSEL_ATOMIC_OPERATOION_LIST_H_

#include "dpsDef.hpp"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   class requestContext;

   class atomicOperationList : public SDBObject
   {
      public:
         atomicOperationList();
         ~atomicOperationList();

         atomicOperationList(const atomicOperationList &) = delete;
         atomicOperationList &operator=(const atomicOperationList &) = delete;


      public:
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return _list.empty();
         }
         OSS_INLINE BOOLEAN isWaitingTail()const
         {
            return _waitingTail;
         }
         OSS_INLINE BOOLEAN isWatingHead()const
         {
            return _list.empty();
         }
         OSS_INLINE BOOLEAN isReadonly()const
         {
            return _nomorePushing;
         }
      public:
         void abort(requestContext *context);

         INT32 push(DPS_LSN_OFFSET lsn);

         DPS_LSN_OFFSET getOplistLsn()const;

         void fini();

         DPS_LSN_OFFSET getLastLsn()const;

         void setWaitingTail();

      private:
         typedef ossPoolList<DPS_LSN_OFFSET> _LSN_LIST;

      public:
         typedef ossPoolList<DPS_LSN_OFFSET>::const_reverse_iterator REVERSE_ITERATOR;
         REVERSE_ITERATOR rbegin()
         {
            return _list.rbegin();
         }
         REVERSE_ITERATOR rend()
         {
            return _list.rend();
         }
         UINT32 getListSize()const
         {
            return _list.size();
         }

      
      private:
         _LSN_LIST _list;
         BOOLEAN _waitingTail = FALSE;
         BOOLEAN _nomorePushing = FALSE;

   };//class atomicOperationList
}//namespace vessel
}//namespace engine

#endif//VESSEL_ATOMIC_OPERATOION_LIST_H_
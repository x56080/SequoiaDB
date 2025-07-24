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

   Source File Name = atomicOperationList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
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

   Source File Name = localThreadSharedPointer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOCAL_THREAD_SHARED_POINTER_H_
#define VESSEL_LOCAL_THREAD_SHARED_POINTER_H_

#include "utilPooledObject.hpp"

namespace engine
{
namespace vessel
{
   class localThreadSharedPointer;

   /// To avoid malloc twice, shared obj must inherit from counter.
   /// The implement like std::make_shared is a little bit hard to debug.
   class localThreadSharedCounter : public _utilPooledObject
   {
      friend class localThreadSharedPointer;

      public:
         localThreadSharedCounter(){}
         ~localThreadSharedCounter();
         localThreadSharedCounter(const localThreadSharedCounter &) = delete;
         localThreadSharedCounter &operator=(const localThreadSharedCounter &) = delete;

      private:
         UINT32 _shared = 0;
   };//class localThreadSharedCounter


   class localThreadSharedPointer : public SDBObject
   {
      public:
         localThreadSharedPointer(){}
         ~localThreadSharedPointer();
         localThreadSharedPointer(const localThreadSharedPointer &o);
         localThreadSharedPointer &operator=(const localThreadSharedPointer &o);
      public:

         OSS_INLINE UINT32 getSharedCount()const
         {
            return isValid() ? _counter->_shared : 0;
         }

         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _counter;
         }

         template <typename T>
         T *get()const
         {
            return isValid() ? static_cast<T*>(_counter) : NULL;
         }

         void release();

         void reset(localThreadSharedCounter *counter);

      private:
         localThreadSharedCounter *_counter = NULL;
   };//class localThreadSharedPointer
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOCAL_THREAD_SHARED_POINTER_H_
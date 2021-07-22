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

   Source File Name = localThreadSharedPointer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/localThreadSharedPointer.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{

////////////////////////////localThreadSharedCounter
   localThreadSharedCounter::~localThreadSharedCounter()
   {
      SDB_ASSERT(0 == _shared, "must be zero");      
   }

////////////////////////////localThreadSharedPointer
   localThreadSharedPointer::~localThreadSharedPointer()
   {
      release();
   }

   localThreadSharedPointer::localThreadSharedPointer(const localThreadSharedPointer &o):
   _counter(NULL)
   {
      if (o.isValid())
      {
         _counter = o._counter;
         ++_counter->_shared;
      }
   }
   
   localThreadSharedPointer &localThreadSharedPointer::operator=(const localThreadSharedPointer &o)
   {
      release();
      if (o.isValid())
      {
         _counter = o._counter;
         ++_counter->_shared;
      }
      return *this;
   }

   void localThreadSharedPointer::release()
   {
      if (isValid())
      {
         SDB_ASSERT(0 < _counter->_shared, "impossible");
         if (0 == (--_counter->_shared))
         {
            SDB_OSS_DEL _counter;
         }
         _counter = NULL;
      }
      return;
   }

   void localThreadSharedPointer::reset(localThreadSharedCounter *counter)
   {
      release();
      if (NULL != counter)
      {
         _counter = counter;
         ++_counter->_shared;
      }
      return;
   }
}//namespace vessel
}//namespace engine
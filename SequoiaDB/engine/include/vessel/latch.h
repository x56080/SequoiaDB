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

   Source File Name = latch.h

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

#ifndef VESSEL_LATCH_H_
#define VESSEL_LATCH_H_

#include "ossTypes.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/interprocess/sync/spin/mutex.hpp>

namespace engine
{
namespace vessel
{
   typedef boost::shared_mutex SHARED_MUTEX;
   typedef boost::mutex UNIQUE_MUTEX;
   typedef boost::interprocess::ipcdetail::spin_mutex SPIN_MUTEX;

   enum LOCK_MODE
   {
      LOCK_MODE_NONE = 0,
      LOCK_MODE_SHARED,
      LOCK_MODE_UPGRADE,
      LOCK_MODE_UNIQUE,
      LOCK_MODE_EXCLUSIVE,
   };
} /// end of namespace vessel
} /// end of namespace engine

#endif
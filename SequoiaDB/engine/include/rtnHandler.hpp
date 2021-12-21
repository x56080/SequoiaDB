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

   Source File Name = rtnHandler.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_RTN_HANDLER_HPP_
#define SDB_RTN_HANDLER_HPP_

#include "sdbInterface.hpp"
#include "pmdEDU.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   class _rtnHandler : public SDBObject
   {
      public:
         _rtnHandler(){}
         virtual ~_rtnHandler(){}

      public:
         virtual INT32 launch(pmdEDUCB *cb) = 0;

      public:
         OSS_INLINE void setAdjunct(const bson::BSONObj &o)
         {
            _adjunct = o;
         }

      protected:
         bson::BSONObj _adjunct;

   };//class _rtnHandler

   typedef class _rtnHandler rtnHandler;
} // namespace engine


#endif//SDB_RTN_HANDLER_HPP_
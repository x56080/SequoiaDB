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

   Source File Name = clsResource.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/14/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_RESOURCE_HPP__
#define CLS_RESOURCE_HPP__

#include "clsRemoteResource.hpp"

namespace engine
{

   /*
      _clsResource define
    */
   class _clsResource : public SDBObject, public _clsRemoteResource
   {
   protected:
      virtual UINT32 _getCataInfoParseGroupID() const
      {
         return _selfNodeID.columns.groupID ;
      }
   } ;
   typedef class _clsResource clsResource ;

}

#endif // CLS_RESOURCE_HPP__

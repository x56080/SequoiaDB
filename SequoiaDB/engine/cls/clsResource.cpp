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

   Source File Name = clsResource.cpp

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

#include "clsResource.hpp"
#include "pmdEDU.hpp"
#include "msgCatalog.hpp"
#include "msgMessageFormat.hpp"
#include "msgMessage.hpp"
#include "coordRemoteHandle.hpp"
#include "coordRemoteSession.hpp"
#include "coordCommon.hpp"
#include "coordFactory.hpp"
#include "pmd.hpp"
#include "rtnCB.hpp"
#include "rtn.hpp"
#include "coordOmProxy.hpp"
#include "coordSequenceAgent.hpp"
#include "coordDataSource.hpp"
#include "coordGTSAgent.hpp"
#include "../bson/bson.h"
#include "utilArray.hpp"

using namespace bson ;

namespace engine
{

}

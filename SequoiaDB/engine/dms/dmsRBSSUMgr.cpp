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

   Source File Name = dmsRBSSUMgr.cpp

   Descriptive Name = DMS Rollback Segment Storage Unit Management

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains code logic for
   rollback segment creation and release.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/24/2019  CYX Initial Draft

   Last Changed =

*******************************************************************************/
#include "dmsStorageUnit.hpp"
#include "../bson/bson.h"
#include "rtn.hpp"
#include "pdTrace.hpp"
#include "dmsTrace.hpp"
#include "pmd.hpp"
#include "dmsCB.hpp"
#include "dmsRBSSUMgr.hpp"
#include "dmsScanner.hpp"
#include "dmsStorageDataCommon.hpp"
#include "dmsStorageDataCapped.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

using namespace bson ;
namespace fs = boost::filesystem ;

namespace engine
{

   _dmsRBSSUMgr::_dmsRBSSUMgr ( SDB_DMSCB *dmsCB ) : _dmsSysSUMgr( dmsCB )
   {
      // By default, start with second collection as the first one stores meta
      _currentCollection = 1 ;
      _lastFreeCollection = DMS_MAX_RBS_CL ;
      // use default size for now, we may want to add config parm later on
      _maxCollectionSize = DMS_DFT_RBSCL_SIZE ;
   }

}

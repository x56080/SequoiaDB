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

   Source File Name = handlers.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_HANDLERS_H_
#define VESSEL_HANDLERS_H_

#include "vessel/createCSHandler.h"
#include "vessel/listCollectionSpaceHandler.h"
#include "vessel/listCollectionsHandler.h"
#include "vessel/createCLHandler.h"
#include "vessel/dmlHandler.h"
#include "vessel/openCLHandler.h"
#include "vessel/scanCLHandler.h"
#include "vessel/countCLHandler.h"
#include "vessel/createIndexHandler.h"
#include "vessel/indexScanHandler.h"
#include "vessel/removeCSHandler.h"
#include "vessel/removeIndexHandler.h"
#include "vessel/testIndexHandler.h"
#include "vessel/removeCLHandler.h"
#include "vessel/truncateCLHandler.h"
#include "vessel/lobChunkHandler.h"

#endif//VESSEL_HANDLERS_H_
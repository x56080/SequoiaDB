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

   Source File Name = dmsInspect.hpp

   Descriptive Name = Data Management Service Storage Unit Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          26/08/2013  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSINSPECT_HPP__
#define DMSINSPECT_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "dmsExtent.hpp"
#include "dmsRecord.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "dmsStorageData.hpp"
#include "dmsStorageIndex.hpp"
#include "../bson/bson.h"
#include "../bson/bsonobj.h"

#include <deque>

using namespace bson ;
using namespace std ;

namespace engine
{

   class _pmdEDUCB ;

   /*
      _dmsInspect define
   */
   class _dmsInspect : public SDBObject
   {
      public:
         _dmsInspect () {}
         ~_dmsInspect () {}

      public:

         static UINT32 inspectHeader ( void * inBuf,
                                       UINT32 inSize,
                                       CHAR * outBuf,
                                       UINT32 outSize,
                                       UINT32 &pageSize,
                                       UINT32 &pageNum,
                                       UINT32 &segmentSize,
                                       UINT64 &secretValue,
                                       SINT32 &err ) ;

         static UINT32 inspectSME ( void * inBuf,
                                    UINT32 inSize,
                                    CHAR * outBuf,
                                    UINT32 outSize,
                                    const CHAR *expBuffer,
                                    UINT32 pageNum,
                                    SINT32 &hwmPages,
                                    SINT32 &err ) ;

         static UINT32 inspectMME ( void * inBuf,
                                    UINT32 inSize,
                                    CHAR * outBuf,
                                    UINT32 outSize,
                                    const CHAR *pCollectionName,
                                    INT32 maxPages,
                                    vector<UINT16> &collections,
                                    SINT32 &err ) ;

         static UINT32 inspectMB ( void * inBuf,
                                   UINT32 inSize,
                                   CHAR * outBuf,
                                   UINT32 outSize,
                                   const CHAR *pCollectionName,
                                   INT32 expCollectionID,
                                   INT32 maxPages,
                                   vector<UINT16> &collections,
                                   SINT32 &err ) ;

         static UINT32 inspectExtentHeader ( void * inBuf,
                                             UINT32 inSize,
                                             CHAR * outBuf,
                                             UINT32 outSize,
                                             UINT16 collectionID,
                                             SINT32 &err ) ;

         static UINT32 inspectIndexCBExtentHeader ( void * inBuf,
                                                    UINT32 inSize,
                                                    CHAR * outBuf,
                                                    UINT32 outSize,
                                                    UINT16 collectionID,
                                                    SINT32 &err ) ;

         static UINT32 inspectIndexCBExtent (  void * inBuf,
                                               UINT32 inSize,
                                               CHAR * outBuf,
                                               UINT32 outSize,
                                               UINT16 collectionID,
                                               dmsExtentID &root,
                                               SINT32 &err ) ;
   } ;
   typedef _dmsInspect dmsInspect ;


}

#endif //DMSINSPECT_HPP__


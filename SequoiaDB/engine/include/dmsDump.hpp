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

   Source File Name = dmsDump.hpp

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
#ifndef DMSDUMP_HPP__
#define DMSDUMP_HPP__

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

   enum
   {
       TITLE1 = 0,
       TITLE2 =1,
       TITLE3 = 3,
       TITLE4 = 5,
       TITLE5 = 7
   };
   #define DMS_SU_DMP_OPT_HEX                ((UINT32)(0x00000001))
   #define DMS_SU_DMP_OPT_HEX_WITH_ASCII     ((UINT32)(0x00000002))
   #define DMS_SU_DMP_OPT_HEX_PREFIX_AS_ADDR ((UINT32)(0x00000004))
   #define DMS_SU_DMP_OPT_FORMATTED          ((UINT32)(0x00000008))

   class _pmdEDUCB ;

   /*
      _dmsDump define
   */
   class _dmsDump : public SDBObject
   {
      public:
         _dmsDump () {}
         ~_dmsDump () {}

      public:
         static UINT32 dumpHeader ( void * inBuf,
                                    UINT32 inSize,
                                    CHAR * outBuf,
                                    UINT32 outSize,
                                    CHAR * addrPrefix,
                                    UINT32 options,
                                    UINT32 &pageSize,
                                    UINT32 &pageNum ) ;

         static UINT32 dumpSME ( void * inBuf,
                                 UINT32 inSize,
                                 CHAR * outBuf,
                                 UINT32 outSize,
                                 UINT32 pageNum ) ;

         static UINT32 dumpMME ( void * inBuf,
                                 UINT32 inSize,
                                 CHAR * outBuf,
                                 UINT32 outSize,
                                 CHAR * addrPrefix,
                                 UINT32 options,
                                 const CHAR *collectionName,
                                 std::vector<UINT16> &collections,
                                 BOOLEAN force ) ;

         static UINT32 dumpMB( void * inBuf,
                               UINT32 inSize,
                               CHAR * outBuf,
                               UINT32 outSize,
                               CHAR * addrPrefix,
                               UINT32 options,
                               const CHAR *collectionName,
                               std::vector<UINT16> &collections,
                               BOOLEAN force ) ;

         static UINT32 dumpRawPage ( void * inBuf,
                                     UINT32 inSize,
                                     CHAR * outBuf,
                                     UINT32 outSize ) ;

         static UINT32 dumpExtentHeader ( void * inBuf,
                                          UINT32 inSize,
                                          CHAR * outBuf,
                                          UINT32 outSize ) ;

         static UINT32 dumpIndexCBExtentHeader ( void * inBuf,
                                                 UINT32 inSize,
                                                 CHAR * outBuf,
                                                 UINT32 outSize ) ;

         static UINT32 dumpIndexCBExtent (  void * inBuf,
                                            UINT32 inSize,
                                            CHAR * outBuf,
                                            UINT32 outSize,
                                            CHAR * addrPrefix,
                                            UINT32 options,
                                            dmsExtentID &root ) ;

      private:
         static UINT32 _dumpExtentHeaderComm( const dmsExtent *extent,
                                              CHAR *outBuf, UINT32 outSize ) ;
   } ;
   typedef _dmsDump dmsDump ;

}

#endif //DMSDUMP_HPP__


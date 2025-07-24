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

   Source File Name = dmsLocalSUMgr.hpp

   Descriptive Name = DMS Local Storage Unit Management Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS Temporary Storage Unit Management.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/24/2020  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMSLOCALSUMGR_HPP__
#define DMSLOCALSUMGR_HPP__

#include "dmsSysSUMgr.hpp"

using namespace bson ;

namespace engine
{

   #define DMS_SYSLOCAL_CS_NAME           "SYSLOCAL"
   #define DMS_SYSLOCALTASK_CL_NAME       "SYSLOCAL.SYSTASK"
   #define DMS_SYSLOCALRECYCLEITEM_CL_NAME "SYSLOCAL.SYSRECYCLEITEMS"

   #define DMS_LOCALTASK_QUERY_DFT_SZ     (1000)

   class _pmdEDUCB ;
   class _dpsLogWrapper ;
   class _SDB_RTNCB ;

   /*
      _dmsLocalSUMgr define
   */
   class _dmsLocalSUMgr : public _dmsSysSUMgr
   {
      public :
         _dmsLocalSUMgr ( _SDB_DMSCB *dmsCB ) ;
         ~_dmsLocalSUMgr() ;

         INT32          init() ;
         void           fini() ;

         INT32          addTask( const BSONObj &obj,
                                 _pmdEDUCB *cb,
                                 _dpsLogWrapper *dpsCB,
                                 INT16 w,
                                 BSONObj &retIDObj ) ;

         INT32          removeTask( const BSONObj &matcher,
                                    _pmdEDUCB *cb,
                                    _dpsLogWrapper *dpsCB,
                                    INT16 w ) ;

         INT32          queryTask( const BSONObj &matcher,
                                   ossPoolVector<BSONObj> &vecObj,
                                   _SDB_RTNCB *rtnCB,
                                   _pmdEDUCB *cb,
                                   INT64 limit = DMS_LOCALTASK_QUERY_DFT_SZ ) ;

         INT32          countTask( const BSONObj &matcher,
                                   _SDB_RTNCB *rtnCB,
                                   _pmdEDUCB *cb,
                                   INT64 &count ) ;

         const CHAR*    getLocalTaskCLName() const ;

      protected:
         INT32          _ensureMetadata ( _pmdEDUCB *cb ) ;

      private:

   } ;
   typedef _dmsLocalSUMgr dmsLocalSUMgr ;

}

#endif //DMSLOCALSUMGR_HPP__


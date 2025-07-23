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

   Source File Name = clsDCMgr.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/02/2015  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DC_CLS_MGR_HPP__
#define DC_CLS_MGR_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "ossRWMutex.hpp"
#include "utilUniqueID.hpp"
#include "utilRecycleBinConf.hpp"
#include "sdbInterface.hpp"
#include <string>

using namespace bson ;
using namespace std ;

namespace engine
{

   /*
      _clsDCBaseInfo define
   */
   class _clsDCBaseInfo : public SDBObject
   {
      public:
         _clsDCBaseInfo() ;
         ~_clsDCBaseInfo() ;

         INT32          lock_r( INT32 millisec = -1 ) ;
         INT32          release_r() ;
         INT32          lock_w( INT32 millisec = -1 ) ;
         INT32          release_w() ;

         INT32          updateFromBSON( BSONObj &obj,
                                        BOOLEAN checkGroups = FALSE ) ;

         void           setClusterName( const string &name ) ;
         void           setBusinessName( const string &name ) ;
         void           setAddress( const string &addr ) ;

         void           setAcitvated( BOOLEAN activated ) ;
         void           setReadonly( BOOLEAN readonly ) ;

      public:
         BOOLEAN        isActivated() const { return _activated ; }
         BOOLEAN        isReadonly() const { return _readonly ; }

         BOOLEAN        hasCSUniqueHWM() const { return _hasCsUniqueHWM ; }
         utilCSUniqueID getCSUniqueHWM() const { return _csUniqueHWM ; }
         UINT32         getCATVersion() const { return _catVersion ; }

         const CHAR*    getClusterName() const ;
         const CHAR*    getBusinessName() const ;
         const CHAR*    getAddress() const ;

         BSONObj        getOrgObj() const { return _orgObj ; }

         utilRecycleBinConf getRecycleBinConf() ;
         void setRecycleBinConf( const utilRecycleBinConf &conf ) ;

      protected:
         void           _reset() ;

      private:
         BSONObj        _orgObj ;

         string         _clusterName ;
         string         _businessName ;
         string         _address ;
         BOOLEAN        _activated ;
         BOOLEAN        _readonly ;
         BOOLEAN        _hasCsUniqueHWM ;
         utilCSUniqueID _csUniqueHWM ;
         UINT32         _catVersion ;

         utilRecycleBinConf _recycleBinConf ;

         ossRWMutex     _rwMutex ;

   } ;
   typedef _clsDCBaseInfo clsDCBaseInfo ;

   /*
      _clsDCMgr define
   */
   class _clsDCMgr :  public SDBObject
   {
      public:
         _clsDCMgr() ;
         ~_clsDCMgr() ;

         INT32          initialize() ;

         clsDCBaseInfo* getDCBaseInfo() { return &_baseInfo ; }

         /*
            Update data center base info from obj
         */
         INT32 updateDCBaseInfo( BSONObj &obj ) ;
         INT32 updateDCBaseInfo( MsgOpReply* pRes ) ;

      private:
         clsDCBaseInfo                 _baseInfo ;       // this dc base info

   } ;

   typedef _clsDCMgr clsDCMgr ;
}

#endif //DC_CLS_MGR_HPP__


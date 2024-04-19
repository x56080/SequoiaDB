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


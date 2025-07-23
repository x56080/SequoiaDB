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

   Source File Name = sptUsrSystem.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_USRSYSTEM_HPP_
#define SPT_USRSYSTEM_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptApi.hpp"

namespace engine
{

   class _sptUsrSystem : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrSystem )

   public:
      _sptUsrSystem() ;
      virtual ~_sptUsrSystem() ;

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail) ;

      INT32 destruct() ;

      INT32 getInfo( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      INT32 memberHelp( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      static INT32 getObj( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail ) ;

      static INT32 ping( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      static INT32 type( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      static INT32 getReleaseInfo( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail ) ;

      static INT32 getHostsMap( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 getAHostMap( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 addAHostMap( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 delAHostMap( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 getCpuInfo( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      static INT32 snapshotCpuInfo( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail ) ;

      static INT32 getMemInfo( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      static INT32 snapshotMemInfo( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail ) ;

      static INT32 getDiskInfo( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 snapshotDiskInfo( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail ) ;

      static INT32 getNetcardInfo( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail ) ;

      static INT32 snapshotNetcardInfo( const _sptArguments &arg,
                                        _sptReturnVal &rval,
                                        bson::BSONObj &detail ) ;

      static INT32 getIpTablesInfo( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail ) ;

      static INT32 getHostName( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 sniffPort ( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      static INT32 getPID ( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      static INT32 getTID ( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      static INT32 getEWD ( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      static INT32 listProcess( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 killProcess( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 addUser( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      static INT32 addGroup( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail ) ;

      static INT32 setUserConfigs( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail ) ;

      static INT32 delUser( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      static INT32 delGroup( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail ) ;

      static INT32 listLoginUsers( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail ) ;

      static INT32 listAllUsers( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail ) ;

      static INT32 listGroups( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      static INT32 getCurrentUser( const _sptArguments &arg,
                                   _sptReturnVal &rval,
                                   bson::BSONObj &detail ) ;

      static INT32 getSystemConfigs( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail ) ;

      static INT32 getProcUlimitConfigs( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail ) ;

      static INT32 setProcUlimitConfigs( const _sptArguments &arg,
                                         _sptReturnVal &rval,
                                         bson::BSONObj &detail ) ;

      static INT32 runService( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail ) ;

      static INT32 createSshKey( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail ) ;

      static INT32 getHomePath( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 getUserEnv( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      static INT32 staticHelp( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

   private:
      static INT32 _extractEnvInfo( const CHAR *buf,
                                    bson::BSONObjBuilder &builder ) ;

      static INT32 _getHomePath( string &homePath ) ;
   } ;
   typedef class _sptUsrSystem sptUsrSystem ;
}
#endif


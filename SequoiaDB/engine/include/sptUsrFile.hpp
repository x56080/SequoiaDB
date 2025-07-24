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

   Source File Name = sptUsrFile.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_USRFILE_HPP_
#define SPT_USRFILE_HPP_

#include "sptApi.hpp"
#include "ossIO.hpp"

namespace engine
{
   class _sptUsrFile : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrFile )

   public:
      _sptUsrFile() ;
      virtual ~_sptUsrFile() ;

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail) ;

      INT32 destruct() ;

      INT32 read( const _sptArguments &arg,
                  _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 write( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 seek( const _sptArguments &arg,
                  _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 close( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 getInfo( const _sptArguments &arg,
                     _sptReturnVal &rval,
                     bson::BSONObj &detail ) ;

      static INT32 remove( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail ) ;

      static INT32 exist( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      static INT32 copy( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      static INT32 move( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      static INT32 mkdir( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      INT32 toString( const _sptArguments &arg,
                      _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 memberHelp( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      static INT32 readFile( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail ) ;

      static INT32 getFileObj( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      static INT32 staticHelp( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      static INT32 find( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      static INT32 chmod( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      static INT32 chown( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      static INT32 chgrp( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      static INT32 getUmask( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail ) ;

      static INT32 setUmask( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail ) ;

      static INT32 list( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      static INT32 isEmptyDir( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      static INT32 getStat( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      static INT32 md5( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      static INT32 getPathType( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

   private:
      static INT32 _extractListInfo( const CHAR* buf,
                                     bson::BSONObjBuilder &builder,
                                     BOOLEAN showDetail ) ;

      static INT32 _extractFindInfo( const CHAR* buf,
                                     bson::BSONObjBuilder &builder ) ;

   private:
      OSSFILE _file ;
      string  _filename ;
   } ;
}

#endif

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
          26/08/2019  HJW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_USRINIFILE_HPP_
#define SPT_USRINIFILE_HPP_

#include "sptApi.hpp"
#include "utilIniParserEx.hpp"

namespace engine
{
   class _sptUsrIniFile : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrIniFile )

   public:
      _sptUsrIniFile() ;
      virtual ~_sptUsrIniFile() ;

   public:
      INT32 construct( const _sptArguments &arg, _sptReturnVal &rval,
                       bson::BSONObj &detail) ;

      INT32 destruct() ;

      INT32 setValue( const _sptArguments &arg, _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;
      INT32 getValue( const _sptArguments &arg, _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 setSectionComment( const _sptArguments &arg, _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;
      INT32 getSectionComment( const _sptArguments &arg, _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      INT32 setComment( const _sptArguments &arg, _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 getComment( const _sptArguments &arg, _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;


      INT32 setLastComment( const _sptArguments &arg, _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      INT32 getLastComment( const _sptArguments &arg, _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      INT32 enableItem( const _sptArguments &arg, _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      INT32 disableItem( const _sptArguments &arg, _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 disableAllItem( const _sptArguments &arg, _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      INT32 toString( const _sptArguments &arg, _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 toObj( const _sptArguments &arg, _sptReturnVal &rval,
                   bson::BSONObj &detail ) ;

      INT32 save( const _sptArguments &arg, _sptReturnVal &rval,
                  bson::BSONObj &detail ) ;

      INT32 getFileName( const _sptArguments &arg, _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;

      INT32 getFlags( const _sptArguments &arg, _sptReturnVal &rval,
                      bson::BSONObj &detail ) ;

      INT32 convertComment( const _sptArguments &arg, _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      INT32 comment2String( const _sptArguments &arg, _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

   private:
      INT32 _exist( const string &path, BOOLEAN &isExist ) ;
      void _setError( bson::BSONObj &detail, const CHAR *errMsg ) ;

   private:
      UINT32 _flags ;
      string _fileName ;
      utilIniParserEx _parser ;
   } ;
}

#endif

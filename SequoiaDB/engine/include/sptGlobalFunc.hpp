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

   Source File Name = sptGlobalFunc.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPT_BLOBALFUNC_HPP_
#define SPT_BLOBALFUNC_HPP_

#include "sptApi.hpp"

namespace engine
{
   class _sptGlobalFunc : public SDBObject
   {
   JS_DECLARE_CLASS( _sptGlobalFunc )

   public:
      _sptGlobalFunc() {}
      virtual ~_sptGlobalFunc() {}

   public:
      static INT32 getLastErrorMsg( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail ) ;

      static INT32 setLastErrorMsg( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail ) ;

      static INT32 getLastError( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail ) ;

      static INT32 setLastError( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail ) ;

      static INT32 setLastErrorObj( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail ) ;

      static INT32 getLastErrorObj( const _sptArguments &arg,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail ) ;

      static INT32 print( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      static INT32 sleep( const _sptArguments &arg,
                          _sptReturnVal &rval,
                          bson::BSONObj &detail ) ;

      static INT32 traceFmt( const _sptArguments &arg,
                             _sptReturnVal &rval,
                             bson::BSONObj &detail ) ;

      static INT32 globalHelp( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      static INT32 displayMethod( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  BSONObj &detail ) ;

      static INT32 displayManual( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  BSONObj &detail ) ;

      static INT32 showClass( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail ) ;

      static INT32 showClassfull( const _sptArguments &arg,
                                  _sptReturnVal &rval,
                                  bson::BSONObj &detail ) ;

      static INT32 forceGC( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      static INT32 importJSFile( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 bson::BSONObj &detail ) ;

      static INT32 importJSFileOnce( const _sptArguments &arg,
                                     _sptReturnVal &rval,
                                     bson::BSONObj &detail ) ;

      static INT32 getExePath( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               bson::BSONObj &detail ) ;

      static INT32 getSelfPath( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 getRootPath( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail ) ;

      static INT32 catPath( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

   protected:
      static INT32 _showClassInner( const _sptArguments &arg,
                                    const string &className,
                                    BOOLEAN showHide,
                                    _sptReturnVal &rval,
                                    bson::BSONObj &detail ) ;
      static INT32 _evalFile( BOOLEAN importOnce,
                              const _sptArguments &arg,
                              _sptReturnVal &rval,
                              bson::BSONObj &detail ) ;
   } ;
   typedef class _sptGlobalFunc sptGlobalFunc ;
}

#endif


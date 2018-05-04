/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = expCLFile.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who          Description
   ====== =========== ============ =============================================
          29/07/2016  Lin Yuebang  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef EXP_CLFILE_HPP_
#define EXP_CLFILE_HPP_

#include "oss.hpp"
#include "ossIO.hpp"
#include "../client/bson/bson.h"
#include "../client/client.h"
#include "../util/utilDecodeRawbson.hpp"
#include "expOptions.hpp"
#include "expCL.hpp"
#include <string>

namespace exprt
{
   using namespace std ;

   class expConvertor : public SDBObject
   {
   public :
      expConvertor() : _buf(NULL), _bufSize(0) {}
      virtual ~expConvertor() { _freeBuf() ; }
      virtual INT32 init() { return SDB_OK ; }
      virtual INT32 head( const CHAR *&toBuf, UINT32 &toSize ) 
      {
         toBuf = NULL ;
         toSize = 0 ;
         return SDB_OK ;
      }
      virtual INT32 tail( const CHAR *&toBuf, UINT32 &toSize ) 
      {
         toBuf = NULL ;
         toSize = 0 ;
         return SDB_OK ;
      }
      virtual INT32 convert( bson &fromRecord, 
                             const CHAR *&toBuf, 
                             UINT32 &toSize ) = 0 ;
   protected :
      CHAR *_getBuf( UINT32 reqSize ) ;
      void  _freeBuf() ;
   protected :
      CHAR     *_buf ;
      UINT32    _bufSize ;
   } ;

   class expJsonConvertor : public expConvertor
   {
   public :
      expJsonConvertor( const expOptions &options, const expCL &cl ) :
         _fieldsBuf(NULL), _options(options), _cl(cl) {}
      virtual INT32 init() ;
      virtual INT32 convert( bson &fromRecord, 
                             const CHAR *&toBuf, 
                             UINT32 &toSize )  ;
   protected :
      void  _freeFieldsBuf() ;
   protected :
      utilDecodeBson    _decodeBson ;
      CHAR             *_fieldsBuf ;
      const expOptions &_options ;
      const expCL      &_cl ;
   } ;

   class expCSVConvertor : public expJsonConvertor
   {
   public :
      expCSVConvertor( const expOptions &options, const expCL &cl ) :
         expJsonConvertor( options, cl ) {}
      virtual INT32 head( const CHAR *&toBuf, UINT32 &toSize ) ;
      virtual INT32 convert( bson &fromRecord, 
                             const CHAR *&toBuf, 
                             UINT32 &toSize )  ;
   } ;

   class expCLOutput : public SDBObject
   {
   public :
      virtual ~expCLOutput() {}
      virtual INT32 open() { return SDB_OK ; }
      virtual void  close() {}
      virtual INT32 output( const CHAR *buf, UINT32 size ) = 0 ;
   } ;

   class expCLFile : public expCLOutput
   {
   public :
      expCLFile( const expOptions &options, const string &fileName ) ;
      virtual ~expCLFile() ;
      virtual INT32 open() ;
      virtual void  close() ;
      virtual INT32 output( const CHAR *buf, UINT32 sz ) ;
   protected :
      INT32 _nextFile() ;
   protected :
      const expOptions &_options ;
      string            _fileName ;
      OSSFILE           _file ;
      BOOLEAN           _opened ;
      UINT64            _writedSize ;
      UINT32            _fileSuffix ;
   } ;
}
#endif
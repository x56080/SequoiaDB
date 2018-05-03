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

   Source File Name = fmpVM.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/19/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef FMPVM_HPP_
#define FMPVM_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "fmpDef.hpp"
#include "../bson/bson.h"

using namespace bson ;

class _fmpVM : public SDBObject
{
public:
   _fmpVM() ;
   virtual ~_fmpVM() ;

public:
   virtual INT32 init( const BSONObj &param ) ;

//   virtual INT32 compile( const BSONElement &func, const CHAR * ) = 0 ;

   virtual INT32 eval( const BSONObj &func,
                       BSONObj &res ) = 0 ;

   virtual INT32 fetch( BSONObj &res ) = 0 ;

   virtual INT32 initGlobalDB( BSONObj &res ) = 0 ;

   OSS_INLINE BOOLEAN ok() const { return _ok ;}

protected:
   OSS_INLINE void _setContext( SINT64 contextID )
   {
      _contextID = contextID ;
      return ;
   }
   OSS_INLINE SINT64 _getContext(){return _contextID ;}

   OSS_INLINE const BSONObj &_getParam() const
   {
      return _param ;
   }

   OSS_INLINE void _setOK( BOOLEAN isOK )
   {
      _ok = isOK ;
      return ;
   }

private:
   BSONObj _param ;
   SINT64 _contextID ;
   BOOLEAN _ok ;
} ;

#endif


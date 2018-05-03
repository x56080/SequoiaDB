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

#ifndef SPT_USRHASH_HPP_
#define SPT_USRHASH_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptApi.hpp"

#define SPT_USR_HASH_DIGEST          "Digest"

namespace engine
{
   /*
      _sptUsrHash define
   */
   class _sptUsrHash : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrHash )

   public:
      _sptUsrHash() ;
      virtual ~_sptUsrHash() ;

   public:
      static INT32 md5( const _sptArguments &arg,
                        _sptReturnVal &rval,
                        bson::BSONObj &detail ) ;

      static INT32 fileMD5( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            bson::BSONObj &detail ) ;

      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail ) ;
   } ;
   typedef class _sptUsrHash sptUsrHash ;
}

#endif // SPT_USRHASH_HPP_


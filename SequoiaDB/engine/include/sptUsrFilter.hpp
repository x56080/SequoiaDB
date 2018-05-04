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

   Source File Name = sptUsrFilter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/08/2016  WJM  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef SPT_USRFILTER_HPP_
#define SPT_USRFILTER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "sptApi.hpp"


namespace engine
{
   enum SPT_FILTER_MATCH
   {
      SPT_FILTER_MATCH_AND,
      SPT_FILTER_MATCH_OR,
      SPT_FILTER_MATCH_NOT
   } ;

   class _sptUsrFilter : public SDBObject
   {
   JS_DECLARE_CLASS( _sptUsrFilter )

   public:
      _sptUsrFilter() ;
      virtual ~_sptUsrFilter() ;

   public:
      INT32 construct( const _sptArguments &arg,
                       _sptReturnVal &rval,
                       bson::BSONObj &detail) ;

      INT32 destruct() ;

      INT32 match( const _sptArguments &arg,
                   _sptReturnVal &rval,
                   bson::BSONObj &detail) ;

      static INT32 help( const _sptArguments &arg,
                         _sptReturnVal &rval,
                         bson::BSONObj &detail) ;

   private:
      BOOLEAN _match( const bson::BSONObj &obj,
                    const bson::BSONObj &filter,
                    SPT_FILTER_MATCH pred ) ;

      bson::BSONObj _filter ;
   } ;
   typedef class _sptUsrFilter sptUsrFilter ;
}
#endif

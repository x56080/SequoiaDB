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

   Source File Name = rtnAlterDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_ALTERDEF_HPP_
#define RTN_ALTERDEF_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   class _pmdEDUCB ;
   class _dpsLogWrapper ;

   enum RTN_ALTER_TYPE
   {
      RTN_ALTER_INVALID = 0,
      RTN_ALTER_TYPE_DB = 1,
      RTN_ALTER_TYPE_CL = 2,
      RTN_ALTER_TYPE_CS = 3,
      RTN_ALTER_TYPE_DOMAIN = 4,
      RTN_ALTER_TYPE_GROUP = 5,
      RTN_ALTER_TYPE_NODE = 6
   } ;

   class _rtnAlterOptions : public SDBObject
   {
   public:
      /// ignore one alter's exception and continue to run next.
      BOOLEAN ignoreException ;

      _rtnAlterOptions()
      :ignoreException( FALSE )
      {

      }

      void reset()
      {
         ignoreException = FALSE ;
      }
   } ;

   typedef INT32 ( *RTN_ALTER_FUNC )( const CHAR *name,
                                      const bson::BSONObj &pubArgs,
                                      const bson::BSONObj &args,
                                      _pmdEDUCB *cb,
                                      _dpsLogWrapper *dpsCB ) ;

   typedef INT32 ( *RTN_ALTER_VERIFY )( const bson::BSONObj &args ) ;


   enum RTN_ALTER_FUNC_TYPE
   {
      RTN_ALTER_FUNC_INVALID = 0,
      RTN_ALTER_CL_CRT_ID_IDX = 1,
      RTN_ALTER_CL_DROP_ID_IDX = 2
   } ;

   class _rtnAlterFuncObj : public SDBObject
   {
   public:
      const CHAR *name ;
      RTN_ALTER_TYPE objType ;
      RTN_ALTER_FUNC_TYPE type ;
      RTN_ALTER_FUNC func ;
      RTN_ALTER_VERIFY verify ;

      _rtnAlterFuncObj()
      :name( NULL ),
       objType( RTN_ALTER_INVALID ),
       type( RTN_ALTER_FUNC_INVALID ),
       func( NULL ),
       verify( NULL )
      {

      }

      _rtnAlterFuncObj( const CHAR *n,
                        RTN_ALTER_TYPE ot,
                        RTN_ALTER_FUNC_TYPE t,
                        RTN_ALTER_FUNC f,
                        RTN_ALTER_VERIFY v )
      :name( n ),
       objType( ot ),
       type( t ),
       func( f ),
       verify( v )
      {

      }

      BOOLEAN isValid() const
      {
         return NULL != name &&
                RTN_ALTER_INVALID != objType &&
                RTN_ALTER_FUNC_INVALID != type &&
                NULL != func &&
                NULL != verify ;
      }
   } ;
}

#endif


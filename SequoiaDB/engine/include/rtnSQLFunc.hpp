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

   Source File Name = rtnSQLFunc.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTNSQLFUNC_HPP_
#define RTNSQLFUNC_HPP_

#include "core.hpp"
#include "qgmDef.hpp"
#include "pd.hpp"
#include "../bson/bson.h"
#include <vector>

using namespace bson ;

namespace engine
{
   typedef std::vector<BSONElement> RTN_FUNC_PARAMS ;

   class _rtnSQLFunc : public SDBObject
   {
   public:
      _rtnSQLFunc( const CHAR *pName = "" )
      {
         if ( pName )
         {
            _name = pName ;
         }
      }
      virtual ~_rtnSQLFunc()
      {
      }

   public:
      INT32 push( const RTN_FUNC_PARAMS &param  )
      {
         INT32 rc = SDB_OK ;

         if ( _alias.empty() )
         {
            PD_LOG( PDERROR, "not initialized yet." ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         if ( _param.size() != param.size() )
         {
            PD_LOG( PDERROR, "number of param should be %d rather than %d",
                    _param.size(), param.size() ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         rc = _push( param ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      done:
         return rc ;
      error:
        goto done ;
      }

      INT32 init( const _qgmField &alias,
                  const vector<qgmOpField> &param )
      {
         _alias = alias ;
         _param = param ;
         return SDB_OK ;
      }

      const vector<qgmOpField> &param() const
      {
         return _param ;
      }

      const CHAR *name() const
      {
         return _name.c_str() ;
      }

      virtual BOOLEAN isAggr() const { return TRUE ; }
      virtual BOOLEAN isStat() const { return FALSE ; }

      virtual INT32 result( BSONObjBuilder &builder ) = 0 ;

      virtual void clear(){ return ; }

      string toString() const ;

   private:
      virtual INT32 _push( const RTN_FUNC_PARAMS &param ) = 0 ;


   protected:
      std::string _name ;
      _qgmField _alias ;
      vector<qgmOpField> _param ;
   } ;

   typedef class _rtnSQLFunc rtnSQLFunc ;
}

#endif


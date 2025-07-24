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

   Source File Name = rtnAlterRunner.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_ALTERRUNNER_HPP_
#define RTN_ALTERRUNNER_HPP_

#include "rtnAlterDef.hpp"
#include "rtnAlterFuncList.hpp"
#include "rtnAlterJob.hpp"

namespace engine
{
   class _pmdEDUCB ;

   class _rtnAlterRunner : public SDBObject
   {
   public:
      _rtnAlterRunner() ;
      virtual ~_rtnAlterRunner() ;

   public:
      INT32 init( const bson::BSONObj &obj ) ;

      void clear() ;

      INT32 run( _pmdEDUCB *cb, _dpsLogWrapper *dpsCB ) ;

      OSS_INLINE const _rtnAlterJob &getJob() const
      {
         return _job ;
      }

   private:
      INT32 _run( const bson::BSONObj &rpc,
                  _pmdEDUCB *cb,
                  _dpsLogWrapper *dpsCB ) ;

      INT32 _getFunc( const CHAR *name,
                      RTN_ALTER_FUNC &func ) ;

   private:
      _rtnAlterJob _job ;
      _rtnAlterFuncList _fl ;
   } ;
}

#endif


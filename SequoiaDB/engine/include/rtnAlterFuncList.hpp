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

   Source File Name = rtnAlterFuncList.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_ALTERFUNCLIST_HPP_
#define RTN_ALTERFUNCLIST_HPP_

#include "rtnAlterDef.hpp"
#include "ossLatch.hpp"
#include "ossUtil.hpp"
#include "msgDef.h"
#include <list>

namespace engine
{
   class _rtnAlterFuncList : public SDBObject
   {
   private:
      typedef std::list<_rtnAlterFuncObj> FOBJ_LIST ;

      class _rtnAlterFuncListInter
      {
      public:
         _rtnAlterFuncListInter(){ _inited = FALSE ;}
         ~_rtnAlterFuncListInter() {}

      public:
         typedef std::list<_rtnAlterFuncObj> FOBJ_LIST ;

         INT32 getFuncObj( RTN_ALTER_TYPE type,
                           const CHAR *name,
                           _rtnAlterFuncObj &obj ) ;

         INT32 getFuncObj( RTN_ALTER_FUNC_TYPE type,
                           _rtnAlterFuncObj &obj ) ;

         INT32 init() ;

      public:
         FOBJ_LIST _fl ;
         _ossSpinXLatch _latch ;
         BOOLEAN _inited ;
      } ;

   public:
      _rtnAlterFuncList() ; 
      ~_rtnAlterFuncList() ;

   public:
      INT32 getFuncObj( RTN_ALTER_TYPE type,
                        const CHAR *name,
                        _rtnAlterFuncObj &obj ) ;

      INT32 getFuncObj( RTN_ALTER_FUNC_TYPE type,
                           _rtnAlterFuncObj &obj ) ;

   private:
      static _rtnAlterFuncListInter _fl ;
   } ;
   
}

#endif


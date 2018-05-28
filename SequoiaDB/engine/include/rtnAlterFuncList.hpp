/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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


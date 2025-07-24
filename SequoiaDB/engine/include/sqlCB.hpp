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

   Source File Name = sqlCB.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SQLCB_HPP_
#define SQLCB_HPP_

#include "sqlGrammar.hpp"
#include "qgmBuilder.hpp"
#include "sdbInterface.hpp"

namespace engine
{
   class _pmdEDUCB ;
   class _qgmPlanContainer ;
   class _rtnSQLFunc ;

   class _sqlCB : public _IControlBlock
   {
   public:
      _sqlCB() ;
      virtual ~_sqlCB() ;

      virtual SDB_CB_TYPE cbType() const { return SDB_CB_SQL ; }
      virtual const CHAR* cbName() const { return "SQLCB" ; }

      virtual INT32  init () ;
      virtual INT32  active () ;
      virtual INT32  deactive () ;
      virtual INT32  fini () ;

   public:
      INT32 exec( const CHAR *sql, _pmdEDUCB *cb,
                  SINT64 &contextID,
                  BOOLEAN &needRollback,
                  BSONObjBuilder *pBuilder = NULL ) ;

      INT32 getFunc( const CHAR *name,
                     UINT32 paramNum,
                     _rtnSQLFunc *&func ) ;

   private:
      INT32 _createContext( _qgmPlanContainer *container,
                            _pmdEDUCB *cb, SINT64 &contextID ) ;

   private:
      SQL_GRAMMAR _grammar ;
   } ;

   typedef class _sqlCB SQL_CB ;

   /*
      get global sql cb
   */
   SQL_CB* sdbGetSQLCB () ;

}

#endif // SQLCB_HPP_


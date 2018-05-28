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


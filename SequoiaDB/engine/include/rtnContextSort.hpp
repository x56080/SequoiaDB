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

   Source File Name = rtnContextSort.hpp

   Descriptive Name = RunTime Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTNCONTEXTSORT_HPP_
#define RTNCONTEXTSORT_HPP_

#include "rtnContext.hpp"
#include "rtnSorting.hpp"

namespace engine
{
   class _rtnContextSort : public _rtnContextBase
   {
   public:
      _rtnContextSort( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextSort() ;

   public:
      virtual RTN_CONTEXT_TYPE getType() const ;
      virtual _dmsStorageUnit*  getSU () { return NULL ; }
      virtual _optAccessPlan *getPlan() { return _planForExplain ; }

      INT32 open( const BSONObj &orderBy,
                  rtnContext *context,
                  _pmdEDUCB *cb,
                  SINT64 numToSkip = 0,
                  SINT64 numToReturn = -1 ) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   private:
      INT32 _sortData( _pmdEDUCB *cb );
      INT32 _rebuildSrcContext( const BSONObj &orderBy,
                                rtnContext *srcContext ) ;

   private:
      rtnContext* _dataContext ;
      _pmdEDUCB * _eduCB;
      BSONObj _orderby ;
      _ixmIndexKeyGen _keyGen ;
      BOOLEAN _dataSorted ;
      _rtnSorting _sorting ;
      SINT64 _skip ;
      SINT64 _limit ;
      _mthSelector _selector ;
      /// WARNING: do not use this plan to do anything
      ///  except keeping plan for explaining. -- yunwu.
      _optAccessPlan *_planForExplain ;
   } ;
   typedef class _rtnContextSort rtnContextSort ;
}

#endif


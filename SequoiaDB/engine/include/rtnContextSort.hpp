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


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

   Source File Name = rtnContextVse.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef RTN_CONTEXT_VESSEL_HPP_
#define RTN_CONTEXT_VESSEL_HPP_

#include "rtnContext.hpp"
#include "vessel/api/cursorHandler.h"

namespace engine
{
   class _rtnContextVseScan : public _rtnContextBase
   {
      public:
         _rtnContextVseScan();
         virtual ~_rtnContextVseScan();

      public:
         virtual const CHAR *name() const {return "vse_scan";}
         virtual RTN_CONTEXT_TYPE getType() const {return RTN_CONTEXT_VESSEL_SCAN;}
         virtual _dmsStorageUnit* getSU() {return NULL;}

      public:
         virtual INT32 open( _dmsStorageUnit *su, _dmsMBContext *mbContext,
                             _pmdEDUCB *cb, const rtnReturnOptions &returnOptions,
                             const BSONObj *blockObj = NULL,
                             INT32 direction = 1 ) 

      protected:
         virtual INT32 _prepareData(_pmdEDUCB *cb);

      public:
         _SDB_DMSCB *_dmsCB = NULL;
         // rest number of records to expect, -1 means select all
         SINT64                     _numToReturn ;
         // rest number of records need to skip
         SINT64                     _numToSkip ;
         // Original return options, number of skip, etc.
         rtnReturnOptions           _returnOptions ;

         vessel::cursorHandler _handler;
   };//class _rtnContextVseScan
}//namespace engine

#endif//RTN_CONTEXT_VESSEL_HPP_
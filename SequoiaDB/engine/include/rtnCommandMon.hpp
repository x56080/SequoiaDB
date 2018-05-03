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

   Source File Name = rtnCommandMon.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          20/09/2016  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_COMMAND_MON_HPP_
#define RTN_COMMAND_MON_HPP_

#include "rtnCommand.hpp"
#include "aggrBuilder.hpp"

using namespace bson ;

namespace engine
{

   class _pmdEDUCB ;
   class _SDB_DMSCB ;
   class _SDB_RTNCB ;
   class _dpsLogWrapper ;
   class _rtnFetchBase ;

   /*
      _rtnMonInnerBase define
   */
   class _rtnMonInnerBase : public _rtnCommand, public _aggrCmdBase
   {
      protected:
         _rtnMonInnerBase () ;
         virtual ~_rtnMonInnerBase () ;

      public:
         virtual INT32 init ( INT32 flags, INT64 numToSkip, INT64 numToReturn,
                              const CHAR *pMatcherBuff,
                              const CHAR *pSelectBuff,
                              const CHAR *pOrderByBuff,
                              const CHAR *pHintBuff ) ;

         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

      protected:
         INT32           _createFetch( _pmdEDUCB *cb,
                                       _rtnFetchBase **ppFetch ) ;

         virtual INT32   _getFetchType() const = 0 ;
         virtual BOOLEAN _isCurrent() const = 0 ;
         virtual BOOLEAN _isDetail() const = 0 ;
         virtual UINT32  _addInfoMask() const ;
         virtual BSONObj _getOptObj() const ;

      protected:
         INT64                _numToReturn ;
         INT64                _numToSkip ;
         const CHAR           *_matcherBuff ;
         const CHAR           *_selectBuff ;
         const CHAR           *_orderByBuff ;
         const CHAR           *_hintBuff ;

         INT32                _flags ;
   } ;

   /*
      _rtnMonBase define
   */
   class _rtnMonBase : public _rtnMonInnerBase
   {
      protected:
         _rtnMonBase () ;
         virtual ~_rtnMonBase () ;
      public:

         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;


      private:
         virtual const CHAR *getIntrCMDName() = 0 ;

   } ;

}

#endif //RTN_COMMAND_MON_HPP_


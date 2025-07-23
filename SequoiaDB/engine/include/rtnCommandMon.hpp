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
#include "rtnFetchBase.hpp"

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

         _rtnMonInnerBase ( const CHAR* name,
                            RTN_COMMAND_TYPE type,
                            INT32 fetchType,
                            UINT32 infoMask ) :
               _name( name ),
               _type( type ),
               _fetchType( fetchType ),
               _infoMask( infoMask )
         {
            _numToReturn = -1 ;
            _numToSkip = 0 ;
            _matcherBuff = NULL ;
            _selectBuff = NULL ;
            _orderByBuff = NULL ;
            _hintBuff = NULL ;
            _flags = 0 ;
         }

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

         const CHAR * name () { return _name ; }
         RTN_COMMAND_TYPE type () { return _type ; }
         INT32   _getFetchType() const { return _fetchType ; }
         UINT32  _addInfoMask() const { return _infoMask ; }
         virtual BOOLEAN _isCurrent() const = 0 ;
         virtual BOOLEAN _isDetail() const = 0 ;
         virtual BSONObj _getOptObj() const ;

         virtual INT32 _getMonProcessor( IRtnMonProcessorPtr & ptr )
         {
            ptr = IRtnMonProcessorPtr() ;
            return SDB_OK ;
         }

         virtual INT32 _checkPrivileges() const ;

      protected :
         // help functions
         BSONObj _getObjectFromHint ( const CHAR * fieldName ) const ;

      protected:
         INT64                _numToReturn ;
         INT64                _numToSkip ;
         const CHAR           *_matcherBuff ;
         const CHAR           *_selectBuff ;
         const CHAR           *_orderByBuff ;
         const CHAR           *_hintBuff ;
         const CHAR           *_name ;

         INT32                _flags ;
         RTN_COMMAND_TYPE     _type ;
         INT32                _fetchType ;
         UINT32               _infoMask ;
   } ;

   /*
      _rtnMonBase define
   */
   class _rtnMonBase : public _rtnMonInnerBase
   {
      protected:
         _rtnMonBase () ;

         _rtnMonBase(const CHAR* name,
                     const CHAR* intrName,
                     RTN_COMMAND_TYPE type,
                     INT32 fetchType,
                     UINT32 infoMask)
           : _rtnMonInnerBase( name, type, fetchType, infoMask ),
             _intrName(intrName)
         {}

         virtual ~_rtnMonBase () ;

      public:
         virtual INT32 doit ( _pmdEDUCB *cb, _SDB_DMSCB *dmsCB,
                              _SDB_RTNCB *rtnCB, _dpsLogWrapper *dpsCB,
                              INT16 w = 1, INT64 *pContextID = NULL  ) ;

      private:
         const CHAR* _intrName ;

         virtual const CHAR *getIntrCMDName() { return _intrName ; }
   } ;

}

#endif //RTN_COMMAND_MON_HPP_


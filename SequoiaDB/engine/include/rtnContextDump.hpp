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

   Source File Name = rtnContextDump.hpp

   Descriptive Name = RunTime Dump Context Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for Runtime
   Context.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/26/2017  David Li  Split from rtnContext.hpp

   Last Changed =

*******************************************************************************/
#ifndef RTN_CONTEXT_DUMP_HPP_
#define RTN_CONTEXT_DUMP_HPP_

#include "rtnContext.hpp"
#include "rtnFetchBase.hpp"
#include "mthMatchRuntime.hpp"

namespace engine
{
   /*
      _rtnContextDump define
   */
   class _rtnContextDump : public _rtnContextBase,
                           public _mthMatchTreeHolder
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextDump )
      public:
         _rtnContextDump ( INT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextDump () ;

         INT32 open ( const BSONObj &selector, const BSONObj &matcher,
                      INT64 numToReturn = -1, INT64 numToSkip = 0 ) ;

         INT32 monAppend( const BSONObj &result ) ;

         void  setMonFetch( rtnFetchBase *pFetch, BOOLEAN ownned ) ;
         void  setMonProcessor( IRtnMonProcessorPtr monProcessorPtr ) ;

         INT64 getNumToReturn() const { return _numToReturn ; }

      public:
         virtual const CHAR*      name() const ;
         virtual RTN_CONTEXT_TYPE getType () const ;
         virtual _dmsStorageUnit* getSU () { return NULL ; }

      protected:
         virtual INT32  _prepareData( _pmdEDUCB *cb ) ;
         virtual void   _toString( stringstream &ss ) ;
         virtual void   _onDataEmpty () ;

      private:
         // rest number of records to expect, -1 means select all
         SINT64                     _numToReturn ;
         // rest number of records need to skip
         SINT64                     _numToSkip ;

         rtnFetchBase               *_pFetch ;
         BOOLEAN                    _ownnedFetch ;

         IRtnMonProcessorPtr        _monProcessorPtr ;

   } ;
   typedef _rtnContextDump rtnContextDump ;
}

#endif /* RTN_CONTEXT_DUMP_HPP_ */


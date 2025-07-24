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

   /*
      _rtnContextSort define
    */
   class _rtnContextSort : public _rtnContextBase,
                           public _rtnSubContextHolder
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextSort )
   public:
      _rtnContextSort( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextSort() ;

   public:
      virtual const CHAR*      name() const ;
      virtual RTN_CONTEXT_TYPE getType() const ;
      virtual _dmsStorageUnit*  getSU () { return NULL ; }

      virtual UINT32 getSULogicalID() const
      {
         return _rtnSubContextHolder::_subSULogicalID ;
      }

      OSS_INLINE virtual optAccessPlanRuntime * getPlanRuntime ()
      {
         /// WARNING: do not use this plan runtime to do anything
         /// except keeping plan for explaining and performance monitor.
         return &_planRuntime ;
      }

      OSS_INLINE virtual const optAccessPlanRuntime * getPlanRuntime () const
      {
         /// WARNING: do not use this plan runtime to do anything
         /// except keeping plan for explaining and performance monitor.
         return &_planRuntime ;
      }

      OSS_INLINE const rtnContext * getSubContext () const
      {
         return _getSubContext() ;
      }

      OSS_INLINE BOOLEAN isInMemorySort () const
      {
         return _sorting.isInMemorySort() ;
      }

      OSS_INLINE const rtnReturnOptions & getReturnOptions () const
      {
         return _returnOptions ;
      }

      INT32 open( const BSONObj &orderBy,
                  rtnContextPtr &context,
                  _pmdEDUCB *cb,
                  SINT64 numToSkip = 0,
                  SINT64 numToReturn = -1 ) ;

      virtual void setQueryActivity ( BOOLEAN hitEnd ) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   private:
      INT32 _sortData( _pmdEDUCB *cb );
      INT32 _rebuildSrcContext( const BSONObj &orderBy,
                                rtnContext *srcContext ) ;

   private:
      BSONObj _orderby ;
      _ixmIndexKeyGen _keyGen ;
      ixmKeyBuilder   _keyBuilder ;
      BOOLEAN _dataSorted ;
      _rtnSorting _sorting ;
      SINT64 _numToSkip ;
      SINT64 _numToReturn ;
      rtnReturnOptions _returnOptions ;
      optAccessPlanRuntime _planRuntime ;
   } ;
   typedef class _rtnContextSort rtnContextSort ;
}

#endif


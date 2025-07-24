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

   Source File Name = rtnContextListLob.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/19/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_CONTEXTLISTLOB_HPP_
#define RTN_CONTEXTLISTLOB_HPP_

#include "rtnContext.hpp"
#include "rtnLobFetcher.hpp"

namespace engine
{
   class _rtnContextListLob : public _rtnContextBase
   {
   public:
      _rtnContextListLob( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextListLob() ;

   public:
      virtual RTN_CONTEXT_TYPE getType() const { return RTN_CONTEXT_LIST_LOB ; }
      virtual _dmsStorageUnit*  getSU () ;

   public:
      INT32 open( const BSONObj &condition,
                  _pmdEDUCB *cb ) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   private:
      INT32 _getMetaInfo( _pmdEDUCB *cb, BSONObj &obj ) ;
      INT32 _getSequenceInfo( _pmdEDUCB *cb, BSONObj &obj ) ;
      INT32 _reallocate( UINT32 len ) ;
   private:
      _rtnLobFetcher _fetcher ;
      CHAR *_buf ;
      UINT32 _bufLen ;
      std::string _fullName ;
      BOOLEAN _fetchLobHead ;
   } ;
   typedef class _rtnContextListLob rtnContextListLob ;
}

#endif


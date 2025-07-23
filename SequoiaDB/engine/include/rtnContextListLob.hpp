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
#include "monInterface.hpp"

using namespace bson ;

namespace engine
{
   /*
      _rtnContextListLob define
   */
   class _rtnContextListLob : public _rtnContextBase, public _IMonSubmitEvent
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextListLob )
   public:
      _rtnContextListLob( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextListLob() ;

   public:
      virtual const CHAR*      name() const { return "LIST_LOB" ; } ;
      virtual RTN_CONTEXT_TYPE getType() const { return RTN_CONTEXT_LIST_LOB ; }
      virtual _dmsStorageUnit*  getSU () ;

      virtual UINT32 getSULogicalID() const
      {
         return _suLogicalID ;
      }

   public:
      INT32 open( const BSONObj &query, const BSONObj &selector,
                  const BSONObj &hint, INT64 skip,
                  INT64 returnNum, _pmdEDUCB *cb ) ;
   public:
      virtual void onSubmit(const monAppCB & delta) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   private:
      INT32 _getMetaInfo( _pmdEDUCB *cb, BSONObj &obj ) ;
      INT32 _getSequenceInfo( _pmdEDUCB *cb, BSONObj &obj ) ;
      INT32 _reallocate( UINT32 len ) ;
      void  _close( _pmdEDUCB *cb ) ;

      INT32 _parseInfoFromQuery( const BSONObj &match,
                                 ossPoolSet<OID> &oids,
                                 ossPoolSet<UINT32> &groups,
                                 BOOLEAN &isNullCond,
                                 BSONObj &newMatch ) ;

      BOOLEAN _parseOID( const BSONElement &e, ossPoolSet<OID> &oids,
                         BOOLEAN &isNullCond, BOOLEAN onlyOID ) ;
      BOOLEAN _parseGroupID( const BSONElement &e, ossPoolSet<UINT32> &groups,
                             BOOLEAN &isNullCond, BOOLEAN onlyNumber ) ;

      INT32 _parseInfoFromHint( const BSONObj &hint,
                                ossPoolSet<OID> &oids,
                                ossPoolSet<UINT32> &groups ) ;

   private:
      _rtnLobFetcher _fetcher ;
      UINT32 _suLogicalID ;
      CHAR *_buf ;
      UINT32 _bufLen ;
      std::string _fullName ;
      BOOLEAN _fetchLobHead ;
      BSONObj _query ;
      BSONObj _selectorObj ;
      BSONObj _hint ;
      INT64 _skip ;
      INT64 _returnNum ;

      BSONObjBuilder _builder ;

      _mthMatchTree _matchTree ;

      monAppCB      _totalDeltaMonApp ;
   } ;
   typedef class _rtnContextListLob rtnContextListLob ;
}

#endif


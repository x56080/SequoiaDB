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

   Source File Name = clsUniqueIDCheckJob.hpp

   Descriptive Name = CS/CL UniqueID Checking Job Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who     Description
   ====== =========== ======= ==============================================
          06/08/2018  Ting YU Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CLS_UNIQUEID_CHECK_JOB_HPP__
#define CLS_UNIQUEID_CHECK_JOB_HPP__

#include "rtnBackgroundJobBase.hpp"
#include "monDMS.hpp"
#include "clsMgr.hpp"
using namespace std ;

namespace engine
{
   /*
    *  _clsUniqueIDCheckJob define
    */
   class _clsUniqueIDCheckJob : public _rtnBaseJob
   {
      public:
      _clsUniqueIDCheckJob () ;
      virtual ~_clsUniqueIDCheckJob () ;

   public:
      virtual RTN_JOB_TYPE type () const { return RTN_JOB_CLS_UNIQUEID_CHECK ; }

      virtual const CHAR* name () const { return "UniqueID-Check-By-Name" ; }

      virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;

      virtual INT32 doit () ;
   } ;

   typedef _clsUniqueIDCheckJob clsUniqueIDCheckJob ;

   INT32 startUniqueIDCheckJob ( EDUID* pEDUID ) ;

   /*
    *  _clsNameCheckJob define
    */
   class _clsNameCheckJob : public _rtnBaseJob
   {
   public:
      _clsNameCheckJob ( UINT64 opID ) ;
      virtual ~_clsNameCheckJob () ;

   public:
      virtual RTN_JOB_TYPE type () const
      {
         return RTN_JOB_CLS_NAME_CHECK_BY_UNIQUEID ;
      }

      virtual const CHAR* name () const { return "Name-Check-By-UniqueID" ; }

      virtual BOOLEAN muteXOn ( const _rtnBaseJob *pOther ) ;

      virtual INT32 doit () ;

   protected:
      INT32 _renameCSCL( vector<monCSSimple>& csList, BOOLEAN unregCL ) ;

      void _registerCLs( const vector<monCSSimple>& csList ) ;

      void _unregisterCL( const string& clName ) ;

      virtual void _onAttach() ;
      virtual void _onDetach() ;

   private:
      UINT64 _opID ;
      map<string, string> _mapRegisterCL ; // <failed cl name, its maincl name>
      clsFreezingWindow* _pFreezeWindow ;
      shardCB* _pShdMgr ;
      BOOLEAN  _hasBlockGlobal ;
   } ;

   typedef _clsNameCheckJob clsNameCheckJob ;

   INT32 startNameCheckJob ( EDUID* pEDUID = NULL ) ;

}

#endif


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

   Source File Name = rtnGTSAgent.hpp

   Descriptive Name = Runtime Global Transaction Agent

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains background job to update
   global lowTran.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef RTN_GTS_AGENT_HPP__
#define RTN_GTS_AGENT_HPP__

#include "rtnBackgroundJobBase.hpp"
#include "msgCatalog.hpp"
#include "dpsTransID.hpp"
#include "dpsTransCB.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      _rtnGTSAgent define
    */
   // _rtnGTSAgent use to handle global transaction events
   class _rtnGTSAgent
   {
   public:
      _rtnGTSAgent() ;
      virtual ~_rtnGTSAgent() ;

   public:
      // update global lowTran
      // return:
      //    - SDB_OK: update succeed
      //    - SDB_GLOB_LOWTRAN_UNKNOWN: global lowTran is not ready
      // NOTE: will send local lowTran to catalog and get back global lowTran
      virtual INT32 updateGlobLowTran() = 0 ;

      // on attach event
      OSS_INLINE virtual void onAttach( pmdEDUCB *eduCB ) {}
      // on detach event
      OSS_INLINE virtual void onDetach( pmdEDUCB *eduCB ) {}

   protected:
      // get local lowTran
      // NOTE: if no global transaction in this node, will return max value
      //       of transaction SN in local lowTran
      INT32 _getLocalLowTran( DPS_TRANSID_SN &localLowTran ) ;
      // set global lowTran
      INT32 _setGlobLowTran( const DPS_TRANSID_SN &globLowTran ) ;
      // fill GTS lowTran request
      INT32 _fillLowTranRequest( MsgGTSLowTranReq *request,
                                 const DPS_TRANSID_SN &nodeLowTran,
                                 bson::BSONObj &requestObject ) ;
      INT32 _parseLowTranResponse( MsgGTSLowTranRsp *response,
                                   DPS_TRANSID_SN &globLowTran ) ;

   protected:
      dpsTransCB * _transCB ;
   } ;

   typedef class _rtnGTSAgent rtnGTSAgent ;

   /*
       _rtnGTSLowTranJob define
    */
   class _rtnGTSLowTranJob : public _rtnBaseJob
   {
   public:
      _rtnGTSLowTranJob( rtnGTSAgent *gtsAgent ) ;
      virtual ~_rtnGTSLowTranJob() ;

   public:
      OSS_INLINE virtual RTN_JOB_TYPE type() const
      {
         return RTN_JOB_GTS_LOWTRAN ;
      }

      OSS_INLINE virtual const CHAR *name() const
      {
         return "GTSLowTranJob" ;
      }

      OSS_INLINE virtual BOOLEAN muteXOn( const _rtnBaseJob *other )
      {
         // only one lowTran job is allowed
         return ( other->type() == RTN_JOB_GTS_LOWTRAN ) ? TRUE : FALSE ;
      }

      virtual INT32 doit() ;

   protected:
      OSS_INLINE virtual void _onAttach()
      {
         // call on attach event of GTS agent
         if ( NULL != _gtsAgent )
         {
            _gtsAgent->onAttach( eduCB() ) ;
         }
      }

      OSS_INLINE virtual void _onDetach()
      {
         // call on detach event of GTS agent
         if ( NULL != _gtsAgent )
         {
            _gtsAgent->onDetach( eduCB() ) ;
         }
      }

   protected:
      // GTS agent
      rtnGTSAgent * _gtsAgent ;
   } ;

   typedef class _rtnGTSLowTranJob rtnGTSLowTranJob ;

   /*
      function to start GTS lowTran job
    */
   INT32 rtnStartGTSLowTranJob( rtnGTSAgent *gtsAgent, EDUID *eduID ) ;

}

#endif // RTN_GTS_AGENT_HPP__

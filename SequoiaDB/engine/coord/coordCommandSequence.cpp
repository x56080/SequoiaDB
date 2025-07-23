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

   Source File Name = coordCommandSequence.cpp

   Descriptive Name = Coordinator Sequence Command

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/06/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "coordCommandSequence.hpp"
#include "coordSequenceAgent.hpp"
#include "coordCB.hpp"
#include "coordResource.hpp"
#include "msgMessage.hpp"

using namespace bson ;

namespace engine
{
   /*
      _coordCMDInvalidateSequenceCache implement
   */
   COORD_IMPLEMENT_CMD_AUTO_REGISTER( _coordCMDInvalidateSequenceCache,
                                      CMD_NAME_INVALIDATE_SEQUENCE_CACHE,
                                      TRUE ) ;
   _coordCMDInvalidateSequenceCache::_coordCMDInvalidateSequenceCache()
   {
   }

   _coordCMDInvalidateSequenceCache::~_coordCMDInvalidateSequenceCache()
   {
   }

   void _coordCMDInvalidateSequenceCache::_preSet( pmdEDUCB * cb,
                                                   coordCtrlParam & ctrlParam )
   {
      ctrlParam._isGlobal = TRUE ;
      ctrlParam._filterID = FILTER_ID_MATCHER ;
      ctrlParam._emptyFilterSel = NODE_SEL_ALL ;
      ctrlParam.resetRole() ;
      ctrlParam._role[ SDB_ROLE_COORD ] = 1 ;
   }

   UINT32 _coordCMDInvalidateSequenceCache::_getControlMask() const
   {
      return COORD_CTRL_MASK_GLOBAL ;
   }

   IMPLEMENT_CMD_AUTO_REGISTER(_coordInvalidateSequenceCache)
   _coordInvalidateSequenceCache::_coordInvalidateSequenceCache()
   {
      _sequenceID = UTIL_SEQUENCEID_NULL ;
   }

   _coordInvalidateSequenceCache::~_coordInvalidateSequenceCache()
   {

   }

   INT32 _coordInvalidateSequenceCache::spaceNode()
   {
      return CMD_SPACE_NODE_COORD ;
   }

   INT32 _coordInvalidateSequenceCache::init ( INT32 flags,
                                               INT64 numToSkip,
                                               INT64 numToReturn,
                                               const CHAR *pMatcherBuff,
                                               const CHAR *pSelectBuff,
                                               const CHAR *pOrderByBuff,
                                               const CHAR *pHintBuff )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObj obj( pMatcherBuff ) ;
         BSONElement e ;

         // check sequence name
         e = obj.getField( FIELD_NAME_SEQUENCE_NAME ) ;
         if ( String == e.type() )
         {
            _sequenceName = e.String() ;
         }
         else if ( !e.eoo() )
         {
            PD_LOG( PDERROR, "Field[%s] is invalid in obj[%s]",
                    FIELD_NAME_SEQUENCE_NAME,
                    obj.toString( false, false).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            PD_LOG( PDERROR, "Missing field[%s] in obj[%s]",
                    FIELD_NAME_SEQUENCE_NAME,
                    obj.toString( false, false ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         if( obj.hasField( FIELD_NAME_SEQUENCE_ID ) )
         {
            e = obj.getField( FIELD_NAME_SEQUENCE_ID ) ;
            PD_CHECK( e.isNumber(), SDB_INVALIDARG, error, PDERROR,
                      "Field[%s] is invalid in obj[%s]",
                     FIELD_NAME_SEQUENCE_ID,
                     obj.toString( false, false ).c_str() ) ;
            _sequenceID = e.Long() ;
         }
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordInvalidateSequenceCache::doit ( _pmdEDUCB *cb,
                                               SDB_DMSCB *dmsCB,
                                               _SDB_RTNCB *rtnCB,
                                               _dpsLogWrapper *dpsCB,
                                               INT16 w,
                                               INT64 *pContextID )
   {
      coordSequenceAgent* sequenceAgent =
         sdbGetCoordCB()->getResource()->getSequenceAgent() ;
      sequenceAgent->removeCache( _sequenceName, _sequenceID ) ;
      return  SDB_OK ;
   }
}


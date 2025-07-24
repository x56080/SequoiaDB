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

   Source File Name = dpsOperationList.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "dpsOperationList.hpp"
#include "pdTrace.hpp"
#include "dpsTrace.hpp"
#include "ossLikely.hpp"
#include "dpsLogRecordDef.hpp"

namespace engine
{
   _dpsOperationList::_dpsOperationList( _dpsOperationList &&o ) noexcept :
   _status( o._status ),
   _nodes( std::move(o._nodes ) )
   {
      o.reset() ;
   }

   _dpsOperationList &_dpsOperationList::operator=( _dpsOperationList &&o ) noexcept
   {
      _status = o._status ;
      _nodes = std::move( o._nodes ) ;
      o.reset() ;
      return *this ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSOPLIST_INITFROMRECORDS, "_dpsOperationList::initFromRecords" )
   INT32 _dpsOperationList::initFromRecords( ossPoolList<utilUniqueBuffer> &&records )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSOPLIST_INITFROMRECORDS ) ;
      ossPoolList<utilUniqueBuffer> l = std::move( records ) ;
      reset() ;

      while ( !l.empty() )
      {
         utilUniqueBuffer &b = l.front() ;
         rc = append( std::move( b ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append record into list:%d", rc ) ;
            goto error ;
         }

         l.pop_front() ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPSOPLIST_INITFROMRECORDS, rc ) ; 
      return rc ;
   error:
      reset() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSOPLIST_APPEND, "_dpsOperationList::append" )
   INT32 _dpsOperationList::append( utilUniqueBuffer &&buffer )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSOPLIST_APPEND ) ;

      if ( OSS_UNLIKELY(!buffer || buffer.getSize() <= DPS_LOG_HEAD_SIZE) )
      {
         PD_LOG( PDERROR, "invalid record buffer to append" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( OSS_UNLIKELY(isCompleted()) ) 
      {
         PD_LOG( PDERROR, "oplist has already been comleted" ) ;
         rc = SDB_INVALID_OPERATION ;
         goto error ;
      }
      else
      {
         dpsOplistNode node( std::move( buffer ) ) ;
         switch ( _status )
         {
         case DPS_OPLIST_STATUS::START:
            rc = _appendWhenStart( std::move( node ) ) ;
            break;
         case DPS_OPLIST_STATUS::BUILDING:
            rc = _appendWhenBuilding( std::move( node ) ) ;
            break;
         case DPS_OPLIST_STATUS::ROLLING_BACK:
            rc = _appendWhenRollingBack( std::move( node ) ) ;
            break;
         default:
            SDB_ASSERT( FALSE, "already been completed" ) ;
            rc = SDB_INVALID_OPERATION ;
            break;
         }

         if ( SDB_OK != rc ) 
         {
            PD_LOG( PDERROR, "failed to append node to list:%d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__DPSOPLIST_APPEND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSOPLIST__APPEND, "_dpsOperationList::_append" )
   INT32 _dpsOperationList::_append( dpsOplistNode &&node )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSOPLIST__APPEND ) ;
      SDB_ASSERT( node.isValid(), "can not be invalid" ) ;

      try
      {
         _nodes.push_back( std::move( node ) ) ;
      }
      catch( std::exception& e )
      {
         PD_LOG( PDERROR, "failed to append more node:%s", e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DPSOPLIST__APPEND, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSOPLIST__APPWHENSTART, "_dpsOperationList::_appendWhenStart" )
   INT32 _dpsOperationList::_appendWhenStart( dpsOplistNode &&node )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSOPLIST__APPWHENSTART ) ;
      SDB_ASSERT( isStart(), "unexpected status" ) ;
      SDB_ASSERT( node.isValid(), "can not be invalid" ) ;
      SDB_ASSERT( _nodes.empty(), "must be empty" ) ;

      DPS_OPL_NODE_TYPE type = node.getType() ;
      if ( OSS_UNLIKELY(DPS_OPL_NODE_TYPE::HEAD != type) )
      {
         PD_LOG( PDERROR, "invalid node to append with type[%d]", type ) ;
         rc = SDB_DPS_BROKEN_OPL ;
         goto error ;
      }
      else if ( OSS_UNLIKELY(node.hasOplNodeInfo() ||
                             node.hasOplRollbackInfo()) )
      {
         PD_LOG( PDERROR, "invalid record element found in head node" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         rc = _append( std::move( node ) ) ;
         if ( OSS_UNLIKELY( SDB_OK != SDB_OK) )
         {
            PD_LOG( PDERROR, "failed to append node into list:%d", rc ) ;
            goto error ;
         }

         _status = DPS_OPLIST_STATUS::BUILDING ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPSOPLIST__APPWHENSTART, rc ) ;
      return rc ;
   error:
      goto done ; 
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSOPLIST__APPWHENBUILD, "_dpsOperationList::_appendWhenBuilding" )
   INT32 _dpsOperationList::_appendWhenBuilding( dpsOplistNode &&node )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSOPLIST__APPWHENBUILD ) ;
      SDB_ASSERT( isBuilding(), "unexpected status" ) ;
      SDB_ASSERT( node.isValid(), "can not be invalid" ) ;
      SDB_ASSERT( !_nodes.empty(), "can not be invalid" ) ;

      BOOLEAN isRollbackNode = FALSE ;
      BOOLEAN isTailNode = FALSE ;

      rc = _checkNonheadNode( node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "node can not be appended into list:%d", rc ) ;
         goto error ;
      }

      isTailNode = DPS_OPL_NODE_TYPE::TAIL == node.getType() ;

      if ( node.hasOplRollbackInfo() )
      {
         rc = _checkRollbackNode( node ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "rollback node can not be appended into list:%d", rc ) ;
            goto error ;
         }

         isRollbackNode = TRUE ;
      }

      rc = _append( std::move( node ) ) ;
      if ( OSS_UNLIKELY(SDB_OK != rc ) )
      {
         PD_LOG( PDERROR, "failed to append node:%d", rc ) ;
         goto error ;
      }

      if ( isRollbackNode )
      {
         _status = DPS_OPLIST_STATUS::ROLLING_BACK ;
      }

      if ( isTailNode )
      {
         _status = DPS_OPLIST_STATUS::COMPLETED ;
      }
   done:
      PD_TRACE_EXITRC( SDB__DPSOPLIST__APPWHENBUILD, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSOPLIST__APPWHENROLLBACK, "_dpsOperationList::_appendWhenRollingBack" )
   INT32 _dpsOperationList::_appendWhenRollingBack( dpsOplistNode &&node )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSOPLIST__APPWHENROLLBACK ) ;
      SDB_ASSERT( isRollingBack(), "unexpected status" ) ;
      SDB_ASSERT( node.isValid(), "can not be invalid" ) ;
      SDB_ASSERT( !_nodes.empty(), "can not be invalid" ) ;

      BOOLEAN isTailNode = FALSE ;

      rc = _checkNonheadNode( node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "node can not be appended into list:%d", rc ) ;
         goto error ;
      }

      isTailNode = DPS_OPL_NODE_TYPE::TAIL == node.getType() ;

      rc = _checkRollbackNode( node ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "rollback node can not be appended into list:%d", rc ) ;
         goto error ;
      }

      rc = _append( std::move( node ) ) ;
      if ( OSS_UNLIKELY(SDB_OK != rc) )
      {
         PD_LOG( PDERROR, "failed to append node:%d", rc ) ;
         goto error ;
      }

      if ( isTailNode )
      {
         _status = DPS_OPLIST_STATUS::COMPLETED ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPSOPLIST__APPWHENROLLBACK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   const dpsOplistNode *_dpsOperationList::seek( const DPS_LSN &lsn ) const
   {
      SDB_ASSERT( lsn.isValid(), "can not be invalid" ) ;
      const dpsOplistNode *res = nullptr ;
      for ( auto itr = _nodes.cbegin(); itr != _nodes.cend(); ++itr )
      {
         if ( itr->getLSN() == lsn )
         {
            res = &( *itr ) ;
            break ;
         }
      }

      return res ;
   }

   BOOLEAN _dpsOperationList::contains( const DPS_LSN &lsn ) const
   {
      return nullptr != seek( lsn ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSOPLIST__CHKROLLBACKNODE, "_dpsOperationList::_checkRollbackNode" )
   INT32 _dpsOperationList::_checkRollbackNode( const dpsOplistNode &node ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSOPLIST__CHKROLLBACKNODE ) ;
      SDB_ASSERT( node.isValid(), "can not be invalid" ) ;

      BOOLEAN targetFound = FALSE ;
      const dpsOplRollbackInfoEle *re = node.getOplRollbackInfo() ;
      if ( nullptr == re )
      {
         PD_LOG( PDERROR, "necessary node info not found in record" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      for ( auto itr = _nodes.cbegin(); itr != _nodes.cend(); ++itr )
      {
         if ( itr->getLSN() != re->targetLSN )
         {
            continue ;
         }
         else if ( itr->hasOplRollbackInfo() )
         {
            PD_LOG( PDERROR, "node[%lld] can not be a rollback target", itr->getLSN().offset ) ;
            rc = SDB_DPS_BROKEN_OPL ;
            goto error ;
         }
         else
         {
            targetFound = TRUE ;
            break ;
         }
      }

      if ( !targetFound )
      {
         PD_LOG( PDERROR, "target[%lld] not found in oplist", re->targetLSN.offset ) ;
         rc = SDB_DPS_BROKEN_OPL ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__DPSOPLIST__CHKROLLBACKNODE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__DPSOPLIST__CHKNHNODE, "_dpsOperationList::_checkNonheadNode" )
   INT32 _dpsOperationList::_checkNonheadNode( const dpsOplistNode &node ) const
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__DPSOPLIST__CHKNHNODE ) ;
      SDB_ASSERT( node.isValid(), "can not be invalid" ) ;

      {
         DPS_OPL_NODE_TYPE type = node.getType() ;
         if ( DPS_OPL_NODE_TYPE::BODY != type &&
              DPS_OPL_NODE_TYPE::TAIL != type )
         {
            PD_LOG( PDERROR, "invalid node to append with type[%d]", type ) ;
            rc = SDB_DPS_BROKEN_OPL ;
            goto error ;
         }
      }

      {
         const dpsOplNodeEle *info = node.getOplNodeInfo() ;
         if ( nullptr == info )
         {
            PD_LOG( PDERROR, "opl node info not found in record" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else if ( getLSN() != info->oplLSN )
         {
            PD_LOG( PDERROR, "node lsn[%lld] does not match list lsn[%lld]",
                    info->oplLSN.offset, getLSN().offset ) ;
            rc = SDB_DPS_BROKEN_OPL ;
            goto error ;
         }
         else if ( getCurrentTailLSN() != info->preLSN )
         {
            PD_LOG( PDERROR, "node pre lsn[%lld] does not match pre lsn[%lld]",
                    info->preLSN.offset, getCurrentTailLSN().offset ) ;
            rc = SDB_DPS_BROKEN_OPL ;
            goto error ;
         }
      }
   done: 
      PD_TRACE_EXITRC( SDB__DPSOPLIST__CHKNHNODE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

} // namespace engine

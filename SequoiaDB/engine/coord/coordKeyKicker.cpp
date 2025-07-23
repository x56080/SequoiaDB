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

   Source File Name = coordShardKicker.cpp

   Descriptive Name = Runtime Coord Operator

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   general operations on coordniator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/04/2017  XJH Initial Draft
   Last Changed =

*******************************************************************************/
#include "coordKeyKicker.hpp"
#include "pmdEDU.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "ossMemPool.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordKeyKicker implement
   */
   _coordKeyKicker::_coordKeyKicker()
   {
      _pResource = NULL ;
   }

   _coordKeyKicker::~_coordKeyKicker()
   {
   }

   void _coordKeyKicker::bind( coordResource *pResource,
                               const CoordCataInfoPtr &cataPtr )
   {
      _pResource = pResource ;
      _cataPtr = cataPtr ;
   }

   BOOLEAN _coordKeyKicker::_isUpdateReplace( const BSONObj &updator )
   {
      //INT32 rc = SDB_OK ;
      BSONObjIterator iter( updator ) ;
      while ( iter.more() )
      {
         BSONElement beTmp = iter.next() ;
         if ( 0 == ossStrcmp( beTmp.fieldName(),
                              CMD_ADMIN_PREFIX FIELD_OP_VALUE_REPLACE ) )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   UINT32 _coordKeyKicker::_addKeys( const BSONObj &objKey )
   {
      UINT32 count = 0 ;
      BSONObjIterator itr( objKey ) ;
      while( itr.more() )
      {
         BSONElement e = itr.next() ;
         if ( _setKeys.count( e.fieldName() ) > 0 )
         {
            continue ;
         }
         ++count ;
         _setKeys.insert( e.fieldName() ) ;
      }
      return count ;
   }

   /// kick sharding key and auto-increment key of a cl
   /// we kick sharding key anyway, and when doing replace,
   /// if new obj has no auto-increment key, we keep the old key.
   INT32 _coordKeyKicker::_kickKey( const CoordCataInfoPtr &cataInfo,
                                    const BSONObj &updator,
                                    BSONObj &newUpdator,
                                    BOOLEAN &hasShardingKey,
                                    BOOLEAN &hasKeepAutoInc,
                                    BOOLEAN ignoreAutoInc )
   {
      INT32 rc = SDB_OK ;
      UINT32 skSiteID = cataInfo->getShardingKeySiteID() ;
      ossPoolSet< strContainner > doneFields ;

      if ( skSiteID > 0 )
      {
         /// if is the same sharding key
         SiteIDSet::iterator it = _skSiteIDs.find( skSiteID );
         if ( it != _skSiteIDs.end() && ignoreAutoInc )
         {
            newUpdator = updator ;
            hasShardingKey = it->second ;
            hasKeepAutoInc = FALSE ;
            goto done ;
         }
      }

      try
      {
         BSONObjBuilder bobNewUpdator( updator.objsize() ) ;
         BSONObj boShardingKey ;
         const clsAutoIncSet &autoIncSet = cataInfo->getAutoIncSet() ;
         BSONObj subObj ;

         BOOLEAN isReplace = _isUpdateReplace( updator ) ;
         cataInfo->getShardingKey( boShardingKey ) ;

         BSONObjIterator iter( updator ) ;
         while ( iter.more() )
         {
            BSONElement beTmp = iter.next() ;
            if ( beTmp.type() != Object )
            {
               rc = SDB_INVALIDARG;
               PD_LOG( PDERROR, "updator's element must be an Object type:"
                       "updator=%s", updator.toString().c_str() ) ;
               goto error ;
            }

            subObj = beTmp.embeddedObject() ;
            //if replace. leave the keep
            if ( isReplace &&
                 0 == ossStrcmp( beTmp.fieldName(),
                                 CMD_ADMIN_PREFIX FIELD_OP_VALUE_KEEP ) )
            {
               _addKeys( subObj ) ;
               continue ;
            }

            BSONObjBuilder subBuilder( bobNewUpdator.subobjStart(
                                       beTmp.fieldName() ) ) ;
            BSONObjIterator iterField( subObj ) ;
            while( iterField.more() )
            {
               BSONElement beField = iterField.next() ;
               const CHAR *pField = beField.fieldName() ;

               if ( _isKey( pField, boShardingKey ) )
               {
                  hasShardingKey = TRUE ;
               }
               else
               {
                  subBuilder.append( beField ) ;
               }

               if ( !ignoreAutoInc && isReplace &&
                    NULL != autoIncSet.findItem( pField ) )
               {
                  doneFields.insert( pField ) ;
               }
            } // while( iterField.more() )

            subBuilder.done() ;
         } // while ( iter.more() )

         if ( isReplace )
         {
            //generate new $keep by combining boUpdator.$keep & boShardingKey.
            UINT32 count = _addKeys( boShardingKey ) ;
            if ( count > 0 )
            {
               hasShardingKey = TRUE ;
            }

            if ( !ignoreAutoInc )
            {
               clsAutoIncIterator autoIncIt( autoIncSet );
               while ( autoIncIt.more() )
               {
                  const clsAutoIncItem *pItem = autoIncIt.next();
                  if ( doneFields.count( pItem->fieldName() ) == 0 )
                  {
                     _setKeys.insert( pItem->fieldName() );
                     hasKeepAutoInc = TRUE;
                  }
               }
            }

            if ( !_setKeys.empty() )
            {
               BSONObjBuilder keepBuilder( bobNewUpdator.subobjStart(
                                           CMD_ADMIN_PREFIX FIELD_OP_VALUE_KEEP ) ) ;
               SET_KEEPKEY::iterator itKey = _setKeys.begin() ;
               while( itKey != _setKeys.end() )
               {
                  keepBuilder.append( (*itKey)._pStr, (INT32)1 ) ;
                  ++itKey ;
               }
               keepBuilder.done() ;
            }
         } // if ( isReplace )
         newUpdator = bobNewUpdator.obj() ;

         if ( skSiteID > 0 )
         {
            _skSiteIDs.insert(
                       pair< UINT32, BOOLEAN >( skSiteID, hasShardingKey ) ) ;
         }

      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR,"Failed to kick sharding key from the record,"
                  "occured unexpected error: %s", e.what() ) ;

         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   /// kick sharding key and auto-increment key of cl
   INT32 _coordKeyKicker::kickKey( const BSONObj &updator,
                                   BSONObj &newUpdator,
                                   BOOLEAN &isChanged,
                                   pmdEDUCB *cb,
                                   const BSONObj &matcher,
                                   BOOLEAN keepShardingKey )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasShardingKey = FALSE ;
      BOOLEAN hasKeepAutoInc = FALSE ;

      if ( !_cataPtr.get() || (!_cataPtr->isSharded() &&
                               !_cataPtr->hasAutoIncrement()) )
      {
         newUpdator = updator ;
         goto done ;
      }

      /// clear
      _skSiteIDs.clear() ;
      _setKeys.clear() ;

      rc = _kickKey( _cataPtr, updator, newUpdator,
                     hasShardingKey, hasKeepAutoInc ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Kick sharding key failed, rc: %d", rc ) ;
         goto error ;
      }
      if ( keepShardingKey )
      {
         if ( hasShardingKey )
         {
            rc = SDB_UPDATE_SHARD_KEY ;
            PD_LOG( PDERROR, "Sharding key cannot be updated, rc: %d", rc ) ;
            goto error ;
         }
         // can't set isChanged to FALSE, in case it is TRUE
         // when passing to this function.
      }
      else
      {
         if ( hasShardingKey )
         {
            isChanged = TRUE ;
         }
      }
      if ( hasKeepAutoInc )
      {
         isChanged = TRUE ;
      }

      /// When is main collection, need to kick all sub-collection's
      /// sharding key
      if ( _cataPtr->isMainCL() )
      {
         vector< string > subCLLst ;
         vector< string >::iterator iterCL ;
         BSONObj subUpdator = newUpdator ;

         rc = _cataPtr->getMatchSubCLs( matcher, subCLLst ) ;
         if ( SDB_CAT_NO_MATCH_CATALOG == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get matched sub collections failed, rc: %d",
                    rc ) ;
            goto error ;
         }

         iterCL = subCLLst.begin() ;
         while( iterCL != subCLLst.end() )
         {
            rc = _kickShardingKey( *iterCL, subUpdator, newUpdator,
                                   isChanged, cb, keepShardingKey ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Kick sharding key for sub-collection[%s] "
                       "failed, rc: %d", (*iterCL).c_str(), rc ) ;
               goto error ;
            }
            subUpdator = newUpdator ;
            ++iterCL ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /// kick sharding key of subcl
   INT32 _coordKeyKicker::_kickShardingKey( const string &collectionName,
                                            const BSONObj &updator,
                                            BSONObj &newUpdator,
                                            BOOLEAN &isChanged,
                                            pmdEDUCB *cb,
                                            BOOLEAN keepShardingKey )
   {
      INT32 rc = SDB_OK ;
      CoordCataInfoPtr cataPtr ;
      BOOLEAN hasShardingKey = FALSE ;
      BOOLEAN hasKeepAutoInc = FALSE ;

      rc = _pResource->getOrUpdateCataInfo( collectionName.c_str(),
                                            cataPtr,
                                            cb ) ;
      if ( SDB_CLS_COORD_NODE_CAT_VER_OLD == rc )
      {
         /// When the main-collection is old, ignored
         rc = SDB_OK ;
         goto done ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Update collection[%s]'s catalog info failed, "
                 "rc: %d", collectionName.c_str(), rc ) ;
         goto error ;
      }

      /// when subcl isn't partition cl
      if ( !cataPtr->isSharded() )
      {
         goto done ;
      }

      rc = _kickKey( cataPtr, updator, newUpdator,
                     hasShardingKey, hasKeepAutoInc, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( keepShardingKey )
      {
         if ( 1 != cataPtr->getGroupNum() && hasShardingKey )
         {
            // num >= 2, cl which has been splited, not allow sharding key
            rc = SDB_UPDATE_SHARD_KEY ;
            PD_LOG( PDERROR, "When the partition cl falls on two or "
                    "more groups, the update rule don't allow sharding"
                    " key. rc: %d", rc ) ;
            goto error ;
         }
         // can't set isChanged to FALSE, in case it is TRUE
         // when passing to this function.
      }
      else
      {
         if ( hasShardingKey )
         {
            isChanged = TRUE ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordKeyKicker::checkShardingKey( const BSONObj &updator,
                                            BOOLEAN &hasInclude,
                                            _pmdEDUCB *cb,
                                            const BSONObj &matcher )
   {
      INT32 rc = SDB_OK ;

      if ( !_cataPtr.get() || !_cataPtr->isSharded() )
      {
         goto done ;
      }

      /// clear
      _skSiteIDs.clear() ;
      _setKeys.clear() ;

      rc = _checkShardingKey( _cataPtr, updator, hasInclude ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Check sharding key failed, rc: %d", rc ) ;
         goto error ;
      }
      if ( hasInclude )
      {
         goto done ;
      }

      /// When is main collection, need to check all sub-collection's
      /// sharding key
      if ( _cataPtr->isMainCL() )
      {
         vector< string > subCLLst ;
         vector< string >::iterator iterCL ;

         rc = _cataPtr->getMatchSubCLs( matcher, subCLLst ) ;
         if ( SDB_CAT_NO_MATCH_CATALOG == rc )
         {
            rc = SDB_OK ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Get matched sub collections failed, rc: %d",
                    rc ) ;
            goto error ;
         }

         iterCL = subCLLst.begin() ;
         while( iterCL != subCLLst.end() )
         {
            rc = _checkShardingKey( *iterCL, updator, hasInclude, cb ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Check sharding key for sub-collection[%s] "
                       "failed, rc: %d", (*iterCL).c_str(), rc ) ;
               goto error ;
            }
            if ( hasInclude )
            {
               goto done ;
            }
            ++iterCL ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _coordKeyKicker::_checkShardingKey( const CoordCataInfoPtr &cataInfo,
                                             const BSONObj &updator,
                                             BOOLEAN &hasInclude )
   {
      INT32 rc = SDB_OK ;
      UINT32 skSiteID = cataInfo->getShardingKeySiteID() ;

      if ( skSiteID > 0 )
      {
         /// if is the same sharding key
         if ( _skSiteIDs.count( skSiteID ) > 0 )
         {
            goto done ;
         }
         _skSiteIDs.insert( pair< UINT32, BOOLEAN >( skSiteID, TRUE ) ) ;
      }

      try
      {
         BSONObjBuilder bobNewUpdator( updator.objsize() ) ;
         BSONObj boShardingKey ;
         BSONObj subObj ;

         BOOLEAN isReplace = _isUpdateReplace( updator ) ;
         cataInfo->getShardingKey( boShardingKey ) ;

         BSONObjIterator iter( updator ) ;
         while ( iter.more() )
         {
            BSONElement beTmp = iter.next() ;
            if ( beTmp.type() != Object )
            {
               rc = SDB_INVALIDARG;
               PD_LOG( PDERROR, "updator's element must be an Object type:"
                       "updator=%s", updator.toString().c_str() ) ;
               goto error ;
            }

            subObj = beTmp.embeddedObject() ;
            //if replace. leave the keep
            if ( isReplace &&
                 0 == ossStrcmp( beTmp.fieldName(),
                                 CMD_ADMIN_PREFIX FIELD_OP_VALUE_KEEP ) )
            {
               _addKeys( subObj ) ;
               continue ;
            }

            BSONObjIterator iterField( subObj ) ;
            while( iterField.more() )
            {
               BSONElement beField = iterField.next() ;
               const CHAR *pField = beField.fieldName() ;
               if ( _isKey( pField, boShardingKey ) )
               {
                  hasInclude = TRUE ;
                  goto done ;
               }
            } // while( iterField.more() )
         } // while ( iter.more() )

         if ( isReplace && _addKeys( boShardingKey ) > 0 )
         {
            hasInclude = TRUE ;
            goto done ;
         }
      }
      catch ( std::exception &e )
      {
         rc = SDB_INVALIDARG;
         PD_LOG ( PDERROR,"Failed to check the record is include sharding-key,"
                  "occured unexpected error: %s", e.what() ) ;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _coordKeyKicker::_checkShardingKey( const string &collectionName,
                                             const BSONObj &updator,
                                             BOOLEAN &hasInclude,
                                             _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      CoordCataInfoPtr cataPtr ;

      rc = _pResource->getOrUpdateCataInfo( collectionName.c_str(),
                                            cataPtr,
                                            cb ) ;
      if ( SDB_CLS_COORD_NODE_CAT_VER_OLD == rc )
      {
         /// When the main-collection is old, ignored
         rc = SDB_OK ;
         goto done ;
      }
      else if ( rc )
      {
         PD_LOG( PDERROR, "Update collection[%s]'s catalog info failed, "
                 "rc: %d", collectionName.c_str(), rc ) ;
         goto error ;
      }

      if ( !cataPtr->isSharded() )
      {
         goto done ;
      }

      rc = _checkShardingKey( cataPtr, updator, hasInclude ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _coordKeyKicker::_isKey( const CHAR *pField, BSONObj &boKey )
   {
      BSONObjIterator iterKey( boKey ) ;
      BOOLEAN isKey = FALSE ;
      const CHAR *pUpdateKey = NULL ;

      while( iterKey.more() )
      {
         BSONElement beKey = iterKey.next();
         const CHAR *pKey = beKey.fieldName();
         pUpdateKey = pField ;

         while( *pKey == *pUpdateKey && *pKey != '\0' )
         {
            ++pKey ;
            ++pUpdateKey ;
         }

         // shardingkey_fieldName == updator_fieldName
         /// key: { a:1 }  field : { a:1 } or { "a.b":1 }
         /// key: { "a.b":1 } field: { a:1 } or { "a.b":1 } or
         ///                         { "a.b.c":1 }
         if ( *pKey == *pUpdateKey ||
              ( '\0' == *pKey && '.' == *pUpdateKey ) ||
              ( '.' == *pKey && '\0' == *pUpdateKey ) )
         {
            isKey = TRUE ;
            break ;
         }
      }
      return isKey ;
   }

}


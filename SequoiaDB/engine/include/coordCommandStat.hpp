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

   Source File Name = coordCommandStat.hpp

   Descriptive Name = Coord Commands

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2017  XJH Init
   Last Changed =

*******************************************************************************/
#ifndef COORD_COMMAND_STAT_HPP__
#define COORD_COMMAND_STAT_HPP__

#include "coordCommandBase.hpp"
#include "coordFactory.hpp"
#include "ossMemPool.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordCMDStatisticsBase define
   */
   class _coordCMDStatisticsBase : public _coordCommandBase
   {
      public:
         _coordCMDStatisticsBase() ;
         virtual ~_coordCMDStatisticsBase() ;

         virtual INT32 execute( MsgHeader *pMsg,
                                pmdEDUCB *cb,
                                INT64 &contextID,
                                rtnContextBuf *buf ) ;
      private:
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) = 0 ;

         virtual INT32 generateVCLResult( const CHAR *pCLName,
                                          rtnContext *pContext,
                                          pmdEDUCB *cb ) ;

         virtual BOOLEAN openEmptyContext() const { return FALSE ; }

         virtual void    _onParsedCollection( const CHAR *pCollectionName, pmdEDUCB *cb ) {}

      private:
         INT32 _executeOnVCL( const CHAR *pCLName,
                              pmdEDUCB *cb,
                              INT64 &contextID ) ;

      protected:
         BSONObj           _hint ;

   } ;
   typedef _coordCMDStatisticsBase coordCMDStatisticsBase ;

   /*
      _coordCMDGetIndexes define, used by versions before v3.4.5
   */
   class _coordCMDGetIndexesOldVersion : public _coordCMDStatisticsBase
   {
      typedef ossPoolMap< string, BSONObj>      CoordIndexMap ;

      public:
         _coordCMDGetIndexesOldVersion() ;
         virtual ~_coordCMDGetIndexesOldVersion() ;

      private :
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) ;
   } ;
   typedef _coordCMDGetIndexesOldVersion coordCMDGetIndexesOldVersion ;

   /*
      _coordCMDGetCount define
   */
   class _coordCMDGetCount : public _coordCMDStatisticsBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetCount() ;
         virtual ~_coordCMDGetCount() ;

      private :
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) ;
         virtual INT32 generateVCLResult( const CHAR *pCLName,
                                          rtnContext *pContext,
                                          pmdEDUCB *cb ) ;
         virtual BOOLEAN openEmptyContext() const { return TRUE ; }

         virtual void    _onParsedCollection( const CHAR *pCollectionName, pmdEDUCB *cb ) ;
   } ;
   typedef _coordCMDGetCount coordCMDGetCount ;

   /*
      _coordCMDGetDatablocks define
   */
   class _coordCMDGetDatablocks : public _coordCMDStatisticsBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetDatablocks() ;
         virtual ~_coordCMDGetDatablocks() ;
      private :
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) ;
   } ;
   typedef _coordCMDGetDatablocks coordCMDGetDatablocks ;

   /*
      _coordCMDGetQueryMeta define
   */
   class _coordCMDGetQueryMeta : public _coordCMDGetDatablocks
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetQueryMeta() ;
         virtual ~_coordCMDGetQueryMeta() ;
   } ;
   typedef _coordCMDGetQueryMeta coordCMDGetQueryMeta ;

   /*
      _coordCMDGetCollectionDetail define
   */
   class _coordCMDGetCollectionDetail : public _coordCMDStatisticsBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetCollectionDetail() ;
         virtual ~_coordCMDGetCollectionDetail() ;
      private :
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) ;
   } ;
   typedef _coordCMDGetCollectionDetail coordCMDGetCollectionDetail ;

   /*
      _coordCMDGetCLStat define
   */
   class _coordCMDGetCLStat : public _coordCMDStatisticsBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetCLStat() ;
         virtual ~_coordCMDGetCLStat() ;

      private :
         virtual INT32 generateResult( rtnContext *pContext, pmdEDUCB *cb ) ;
      
      private:
         void _merge( const collectionStatInfo &from, collectionStatInfo &to ) ;
   } ;
   typedef _coordCMDGetCLStat coordCMDGetCLStat ;

   /*
      _coordCMDGetIndexStat define
   */
   class _coordCMDGetIndexStat : public _coordCMDStatisticsBase
   {
      COORD_DECLARE_CMD_AUTO_REGISTER() ;
      public:
         _coordCMDGetIndexStat() ;
         virtual ~_coordCMDGetIndexStat() ;
      private :
         virtual INT32 generateResult( rtnContext *pContext,
                                       pmdEDUCB *cb ) ;
         INT32 _getResultCount( UINT32 &resCount, pmdEDUCB *cb ) ;
   } ;
   typedef _coordCMDGetIndexStat coordCMDGetIndexStat ;

}

#endif // COORD_COMMAND_STAT_HPP__

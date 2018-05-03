/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = dmsStorageLoadExtent.hpp

   Descriptive Name =

   When/how to use: load extent to database

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/10/2013  JW  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef DMSSTORAGE_LOADEXTENT_HPP_
#define DMSSTORAGE_LOADEXTENT_HPP_

#include "dmsStorageUnit.hpp"
#include "pmdEDUMgr.hpp"

using namespace bson ;

namespace engine
{
   class migMaster ;

   /*
      dmsStorageLoadOp define
   */
   class dmsStorageLoadOp : public SDBObject
   {
   private:
      CHAR           *_pCurrentExtent ;
      UINT32         _buffSize ;
      INT32           _currentExtentSize ;
      dmsExtent      *_currentExtent ;
      dmsStorageUnit *_su ;
      SINT32          _pageSize ;

   private:
      void  _initExtentHeader ( dmsExtent *extAddr, UINT16 numPages ) ;
      INT32 _allocateExtent   ( INT32 requestSize ) ;

   public:
      dmsStorageLoadOp( ) : _pCurrentExtent(NULL),
                            _buffSize( 0 ),
                            _currentExtentSize(0),
                            _currentExtent(NULL),
                            _su(NULL)
      {
      }

      ~dmsStorageLoadOp()
      {
         SAFE_OSS_FREE ( _pCurrentExtent ) ;
         _buffSize = 0 ;
      }

      // Flag Load
      BOOLEAN isFlagLoad ( dmsMB *mb )
      {
         return DMS_IS_MB_LOAD ( mb->_flag ) ;
      }

      void setFlagLoad ( dmsMB *mb )
      {
         DMS_SET_MB_LOAD ( mb->_flag ) ;
      }

      void clearFlagLoad ( dmsMB *mb )
      {
         OSS_BIT_CLEAR ( mb->_flag, DMS_MB_FLAG_LOAD ) ;
      }

      // Flag Load Load
      BOOLEAN isFlagLoadLoad ( dmsMB *mb )
      {
         return DMS_IS_MB_FLAG_LOAD_LOAD ( mb->_flag ) ;
      }

      void setFlagLoadLoad ( dmsMB *mb )
      {
         DMS_SET_MB_FLAG_LOAD_LOAD ( mb->_flag ) ;
      }

      void clearFlagLoadLoad ( dmsMB *mb )
      {
         OSS_BIT_CLEAR ( mb->_flag, DMS_MB_FLAG_LOAD_LOAD ) ;
      }

      // Flag Load Build
      BOOLEAN isFlagLoadBuild ( dmsMB *mb )
      {
         return DMS_IS_MB_FLAG_LOAD_BUILD ( mb->_flag ) ;
      }

      void setFlagLoadBuild ( dmsMB *mb )
      {
         DMS_SET_MB_FLAG_LOAD_BUILD ( mb->_flag ) ;
      }

      void clearFlagLoadBuild ( dmsMB *mb )
      {
         OSS_BIT_CLEAR ( mb->_flag, DMS_MB_FLAG_LOAD_BUILD ) ;
      }

      void init ( dmsStorageUnit *su )
      {
         SDB_ASSERT ( su, "su is NULL" ) ;
         _su = su ;
         _pageSize = _su->getPageSize() ;
      }

      INT32 pushToTempDataBlock ( dmsMBContext *mbContext,
                                  pmdEDUCB *cb,
                                  BSONObj &record,
                                  BOOLEAN isLast,
                                  BOOLEAN isAsynchr ) ;

      INT32 loadBuildPhase ( dmsMBContext *mbContext,
                             pmdEDUCB *cb,
                             BOOLEAN isAsynchr = FALSE,
                             migMaster *pMaster = NULL,
                             UINT32 *success = NULL,
                             UINT32 *failure = NULL ) ;

      INT32 loadRollbackPhase ( dmsMBContext *mbContext ) ;

   } ;

}

#endif // DMSSTORAGE_LOADEXTENT_HPP_

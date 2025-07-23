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

   Source File Name = dmsExtDataHandler.hpp

   Descriptive Name = External data process handler for dms.

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   DMS storage unit and its methods.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          14/04/2017  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_EXTDATAHANDLER_HPP__
#define DMS_EXTDATAHANDLER_HPP__

#include "oss.hpp"
#include "dpsLogWrapper.hpp"
#include "monDMS.hpp"

namespace engine
{
   class _dmsMBContext ;

   enum _DMS_EXTOPR_TYPE
   {
      DMS_EXTOPR_TYPE_INSERT = 0,
      DMS_EXTOPR_TYPE_DELETE,
      DMS_EXTOPR_TYPE_UPDATE,
      DMS_EXTOPR_TYPE_CRTIDX,
      DMS_EXTOPR_TYPE_TRUNCATE,
      DMS_EXTOPR_TYPE_DROPCS,
      DMS_EXTOPR_TYPE_DROPCL,
      DMS_EXTOPR_TYPE_DROPIDX,
      DMS_EXTOPR_TYPE_REBUILDIDX
   } ;
   typedef enum _DMS_EXTOPR_TYPE DMS_EXTOPR_TYPE ;

   class _IDmsExtDataHandler
   {
   public:
      _IDmsExtDataHandler() {}
      virtual ~_IDmsExtDataHandler() {}

   public:
      virtual INT32 prepare( DMS_EXTOPR_TYPE type, const CHAR *csName,
                             const CHAR *clName, const CHAR *idxName,
                             const BSONObj *object, const BSONObj *objNew,
                             _pmdEDUCB *cb) = 0 ;

      virtual INT32 onOpenTextIdx( const CHAR *csName, const CHAR *clName,
                                   ixmIndexCB &indexCB ) = 0 ;

      virtual INT32 onDelCS( const CHAR *csName, _pmdEDUCB *cb,
                             BOOLEAN removeFiles, SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onDelCL( const CHAR *csName, const CHAR *clName,
                             _pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) = 0;

      virtual INT32 onCrtTextIdx( _dmsMBContext *context, const CHAR *csName,
                                  ixmIndexCB &indexCB, _pmdEDUCB *cb,
                                  SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onDropTextIdx( const CHAR *extName, _pmdEDUCB *cb,
                                   SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onRebuildTextIdx( const CHAR *csName, const CHAR *clName,
                                      const CHAR *idxName, const CHAR *extName,
                                      const BSONObj &idxKeyDef, _pmdEDUCB *cb,
                                      SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onInsert( const CHAR *extName, const BSONObj &object,
                              _pmdEDUCB* cb, SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onDelete( const CHAR *extName, const BSONObj &object,
                              _pmdEDUCB* cb, SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onUpdate( const CHAR *extName, const BSONObj &orignalObj,
                              const BSONObj &newObj, _pmdEDUCB* cb,
                              BOOLEAN isRollback = FALSE,
                              SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onTruncateCL( const CHAR *csName, const CHAR *clName,
                                  _pmdEDUCB *cb, BOOLEAN needChangeCLID = TRUE,
                                  SDB_DPSCB *dpsCB = NULL ) = 0 ;

      virtual INT32 onRenameCS( const CHAR *oldCSName, const CHAR *newCSName,
                                _pmdEDUCB *cb, SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 onRenameCL( const CHAR *csName, const CHAR *oldCLName,
                                const CHAR *newCLName, _pmdEDUCB *cb,
                                SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 done( DMS_EXTOPR_TYPE type, _pmdEDUCB *cb,
                          SDB_DPSCB *dpscb = NULL ) = 0 ;

      virtual INT32 abortOperation( DMS_EXTOPR_TYPE type, _pmdEDUCB *cb ) = 0 ;
   } ;
   typedef _IDmsExtDataHandler IDmsExtDataHandler ;
}

#endif /* DMS_EXTDATAHANDLER_HPP__ */


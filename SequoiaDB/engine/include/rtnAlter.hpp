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

   Source File Name = rtnAlter.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/03/2018  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_ALTER_HPP_
#define RTN_ALTER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossUtil.hpp"
#include "rtnAlterTask.hpp"
#include "dms.hpp"
#include "utilResult.hpp"

namespace engine
{

   class _pmdEDUCB ;
   class _SDB_DMSCB ;
   class _dpsLogWrapper ;
   class _dmsStorageUnit ;
   class _dmsMBContext ;

   INT32 rtnAlterCommand ( const CHAR * name,
                           RTN_ALTER_OBJECT_TYPE objectType,
                           bson::BSONObj alterObject,
                           _pmdEDUCB * cb,
                           _dpsLogWrapper * dpsCB ) ;

   INT32 rtnAlter ( const CHAR * name,
                    const rtnAlterTask * task,
                    const rtnAlterInfo * alterInfo,
                    const rtnAlterOptions * options,
                    _pmdEDUCB * cb,
                    _dpsLogWrapper * dpsCB,
                    utilWriteResult *pResult = NULL ) ;

   INT32 rtnCheckAlterCollection ( const CHAR * collection,
                                   const rtnAlterTask * task,
                                   _pmdEDUCB * cb,
                                   _dmsMBContext * mbContext,
                                   _dmsStorageUnit * su,
                                   _SDB_DMSCB * dmsCB ) ;

   INT32 rtnAlterCollection ( const CHAR * collection,
                              const rtnAlterTask * task,
                              const rtnAlterInfo * alterInfo,
                              const rtnAlterOptions * options,
                              _pmdEDUCB * cb,
                              _dpsLogWrapper * dpsCB,
                              utilWriteResult *pResult = NULL ) ;

   INT32 rtnAlterCollection ( const CHAR * collection,
                              const rtnAlterTask * task,
                              const rtnAlterInfo * alterInfo,
                              const rtnAlterOptions * options,
                              _pmdEDUCB * cb,
                              _dpsLogWrapper * dpsCB,
                              _dmsMBContext * mbContext,
                              _dmsStorageUnit * su,
                              _SDB_DMSCB * dmsCB,
                              utilWriteResult *pResult = NULL ) ;

   INT32 rtnAlterCollectionSpace ( const CHAR * collectionSpace,
                                   const rtnAlterTask * task,
                                   const rtnAlterInfo * alterInfo,
                                   const rtnAlterOptions * options,
                                   _pmdEDUCB * cb,
                                   _dpsLogWrapper * dpsCB ) ;

   INT32 rtnAlterCollectionSpace ( const CHAR * collectionSpace,
                                   const rtnAlterTask * task,
                                   const rtnAlterInfo * alterInfo,
                                   const rtnAlterOptions * options,
                                   _pmdEDUCB * cb,
                                   _dpsLogWrapper * dpsCB,
                                   _dmsStorageUnit * su,
                                   _SDB_DMSCB * dmsCB ) ;

   INT32 _rtnAlter2DPSLog ( const CHAR * name,
                            const rtnAlterTask * task,
                            const rtnAlterInfo * alterInfo,
                            const rtnAlterOptions * options,
                            _pmdEDUCB * cb,
                            _dpsLogWrapper * dpsCB,
                            _dmsMBContext * mbContext,
                            _dmsStorageUnit * su,
                            DMS_FILE_TYPE dpsType,
                            BOOLEAN needToUpdateLsn = FALSE ) ;
}

#endif // RTN_ALTER_HPP_

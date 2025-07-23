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

namespace engine
{

   class _pmdEDUCB ;
   class _SDB_DMSCB ;
   class _dpsLogWrapper ;
   class _dmsStorageUnit ;
   class _dmsMBContext ;

   INT32 rtnAlter ( const CHAR * name,
                    const rtnAlterTask * task,
                    const rtnAlterOptions * options,
                    _pmdEDUCB * cb,
                    _dpsLogWrapper * dpsCB ) ;

   INT32 rtnAlter ( const CHAR * name,
                    RTN_ALTER_OBJECT_TYPE objectType,
                    bson::BSONObj alterObject,
                    _pmdEDUCB * cb,
                    _dpsLogWrapper * dpsCB ) ;

   INT32 rtnAlter2DPSLog ( const CHAR * name,
                           const rtnAlterTask * task,
                           const rtnAlterOptions * options,
                           _dpsLogWrapper * dpsCB ) ;

   INT32 rtnCreateIDIndex ( const CHAR * name,
                            const rtnAlterTask * task,
                            const rtnAlterOptions * options,
                            _pmdEDUCB * cb,
                            _dpsLogWrapper * dpsCB ) ;

   INT32 rtnDropIDIndex ( const CHAR * name,
                          const rtnAlterTask * task,
                          const rtnAlterOptions * options,
                          _pmdEDUCB * cb,
                          _dpsLogWrapper * dpsCB ) ;

   INT32 rtnEnableSharding ( const CHAR * name,
                             const rtnAlterTask * task,
                             const rtnAlterOptions * options,
                             _pmdEDUCB * cb,
                             _dpsLogWrapper * dpsCB ) ;

   INT32 rtnDisableSharding ( const CHAR * name,
                              const rtnAlterTask * task,
                              const rtnAlterOptions * options,
                              _pmdEDUCB * cb,
                              _dpsLogWrapper * dpsCB ) ;

   INT32 rtnCollectionSetSharding ( const CHAR * collection,
                                    const rtnCLShardingArgument & argument,
                                    _pmdEDUCB * cb,
                                    _dmsMBContext * mbContext,
                                    _dmsStorageUnit * su,
                                    _SDB_DMSCB * dmsCB ) ;

   INT32 rtnCollectionCheckSharding ( const CHAR * collection,
                                      const rtnCLShardingArgument & argument,
                                      _pmdEDUCB * cb,
                                      _dmsMBContext * mbContext,
                                      _dmsStorageUnit * su,
                                      _SDB_DMSCB * dmsCB ) ;

   INT32 rtnCollectionSetCompress ( const CHAR * collection,
                                    const rtnCLCompressArgument & argument,
                                    _pmdEDUCB * cb,
                                    _dmsMBContext * mbContext,
                                    _dmsStorageUnit * su,
                                    _SDB_DMSCB * dmsCB ) ;

   INT32 rtnCollectionSetCompress ( const CHAR * collection,
                                    UTIL_COMPRESSOR_TYPE compressorType,
                                    _pmdEDUCB * cb,
                                    _dmsMBContext * mbContext,
                                    _dmsStorageUnit * su,
                                    _SDB_DMSCB * dmsCB ) ;

   INT32 rtnCollectionSetExtOptions ( const CHAR * collection,
                                      const rtnCLExtOptionArgument & cappedArgument,
                                      _pmdEDUCB * cb,
                                      _dmsMBContext * mbContext,
                                      _dmsStorageUnit * su,
                                      _SDB_DMSCB * dmsCB ) ;

   INT32 rtnEnableCompress ( const CHAR * name,
                             const rtnAlterTask * task,
                             const rtnAlterOptions * options,
                             _pmdEDUCB * cb,
                             _dpsLogWrapper * dpsCB ) ;

   INT32 rtnDisableCompress ( const CHAR * name,
                              const rtnAlterTask * task,
                              const rtnAlterOptions * options,
                              _pmdEDUCB * cb,
                              _dpsLogWrapper * dpsCB ) ;

   INT32 rtnAlterCLCheckAttributes ( const CHAR * collection,
                                     const rtnAlterTask * task,
                                     _dmsMBContext * mbContext,
                                     _dmsStorageUnit * su,
                                     _SDB_DMSCB * dmsCB,
                                     _pmdEDUCB * cb ) ;

   INT32 rtnAlterCLSetAttributes ( const CHAR * collection,
                                   const rtnAlterTask * task,
                                   _dmsMBContext * mbContext,
                                   _dmsStorageUnit * su,
                                   _SDB_DMSCB * dmsCB,
                                   _pmdEDUCB * cb ) ;

   INT32 rtnAlterCLSetAttributes ( const CHAR * name,
                                   const rtnAlterTask * task,
                                   const rtnAlterOptions * options,
                                   _pmdEDUCB * cb,
                                   _dpsLogWrapper * dpsCB ) ;

   INT32 rtnAlterCSSetAttributes ( const CHAR * collectionSpace,
                                   const rtnAlterTask * task,
                                   _dmsStorageUnit * su,
                                   _SDB_DMSCB * dmsCB,
                                   _pmdEDUCB * cb ) ;

   INT32 rtnAlterCSSetAttributes ( const CHAR * name,
                                   const rtnAlterTask * task,
                                   const rtnAlterOptions * options,
                                   _pmdEDUCB * cb,
                                   _dpsLogWrapper * dpsCB ) ;

}

#endif // RTN_ALTER_HPP_

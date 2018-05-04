/******************************************************************************

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

   Source File Name = mthSActionFunc.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_SACTIONFUNC_HPP_
#define MTH_SACTIONFUNC_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "mthCommon.hpp"
#include "../bson/bson.hpp"

namespace engine
{
   class _mthSAction ;
   typedef INT32 (*MTH_SACTION_BUILD)( const CHAR *,
                                       const bson::BSONElement &,
                                       _mthSAction *,
                                       bson::BSONObjBuilder & ) ;

   typedef INT32 (*MTH_SACTION_GET)( const CHAR *,
                                     const bson::BSONElement &,
                                     _mthSAction *,
                                     bson::BSONElement & ) ;

   INT32 mthIncludeBuild( const CHAR *,
                          const bson::BSONElement &,
                          _mthSAction *,
                          bson::BSONObjBuilder & ) ;

   INT32 mthIncludeGet( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONElement & ) ;

   INT32 mthDefaultBuild( const CHAR *,
                          const bson::BSONElement &,
                          _mthSAction *,
                          bson::BSONObjBuilder & ) ;

   INT32 mthDefaultGet( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONElement & ) ;

   INT32 mthSliceBuild( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONObjBuilder & ) ;

   INT32 mthSliceGet( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONElement & ) ;

   INT32 mthElemMatchBuild( const CHAR *,
                            const bson::BSONElement &,
                            _mthSAction *,
                            bson::BSONObjBuilder & ) ;

   INT32 mthElemMatchGet( const CHAR *,
                          const bson::BSONElement &,
                          _mthSAction *,
                          bson::BSONElement & ) ;

   INT32 mthElemMatchOneBuild( const CHAR *,
                               const bson::BSONElement &,
                               _mthSAction *,
                               bson::BSONObjBuilder & ) ;

   INT32 mthElemMatchOneGet( const CHAR *,
                             const bson::BSONElement &,
                             _mthSAction *,
                             bson::BSONElement & ) ;

   INT32 mthAbsBuild( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONObjBuilder & ) ;

   INT32 mthAbsGet( const CHAR *,
                    const bson::BSONElement &,
                    _mthSAction *,
                    bson::BSONElement & ) ;

   INT32 mthCeilingBuild( const CHAR *,
                          const bson::BSONElement &,
                          _mthSAction *,
                          bson::BSONObjBuilder & ) ;

   INT32 mthCeilingGet( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONElement & ) ;

   INT32 mthFloorBuild( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONObjBuilder & ) ;

   INT32 mthFloorGet( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONElement & ) ;

   INT32 mthModBuild( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONObjBuilder & ) ;

   INT32 mthModGet( const CHAR *,
                    const bson::BSONElement &,
                    _mthSAction *,
                    bson::BSONElement & ) ;

   INT32 mthCastBuild( const CHAR *,
                       const bson::BSONElement &,
                       _mthSAction *,
                       bson::BSONObjBuilder & ) ;

   INT32 mthCastGet( const CHAR *,
                     const bson::BSONElement &,
                     _mthSAction *,
                     bson::BSONElement & ) ;

   INT32 mthSubStrBuild( const CHAR *,
                         const bson::BSONElement &,
                         _mthSAction *,
                         bson::BSONObjBuilder & ) ;

   INT32 mthSubStrGet( const CHAR *,
                       const bson::BSONElement &,
                       _mthSAction *,
                       bson::BSONElement & ) ;

   INT32 mthStrLenBuild( const CHAR *,
                         const bson::BSONElement &,
                         _mthSAction *,
                         bson::BSONObjBuilder & ) ;

   INT32 mthStrLenGet( const CHAR *,
                       const bson::BSONElement &,
                       _mthSAction *,
                       bson::BSONElement & ) ;

   INT32 mthLowerBuild( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONObjBuilder & ) ;

   INT32 mthLowerGet( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONElement & ) ;

   INT32 mthUpperBuild( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONObjBuilder & ) ;

   INT32 mthUpperGet( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONElement & ) ;

   INT32 mthTrimBuild( const CHAR *,
                       const bson::BSONElement &,
                       _mthSAction *,
                       bson::BSONObjBuilder & ) ;

   INT32 mthTrimGet( const CHAR *,
                     const bson::BSONElement &,
                     _mthSAction *,
                     bson::BSONElement & ) ;

   INT32 mthLTrimBuild( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONObjBuilder & ) ;

   INT32 mthLTrimGet( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONElement & ) ;

   INT32 mthRTrimBuild( const CHAR *,
                        const bson::BSONElement &,
                        _mthSAction *,
                        bson::BSONObjBuilder & ) ;

   INT32 mthRTrimGet( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONElement & ) ;

   INT32 mthAddBuild( const CHAR *,
                      const bson::BSONElement &,
                      _mthSAction *,
                      bson::BSONObjBuilder & ) ;

   INT32 mthAddGet( const CHAR *,
                    const bson::BSONElement &,
                    _mthSAction *,
                    bson::BSONElement & ) ;

   INT32 mthSubtractBuild( const CHAR *,
                           const bson::BSONElement &,
                           _mthSAction *,
                           bson::BSONObjBuilder & ) ;

   INT32 mthSubtractGet( const CHAR *,
                         const bson::BSONElement &,
                         _mthSAction *,
                         bson::BSONElement & ) ;

   INT32 mthMultiplyBuild( const CHAR *,
                           const bson::BSONElement &,
                           _mthSAction *,
                           bson::BSONObjBuilder & ) ;

   INT32 mthMultiplyGet( const CHAR *,
                         const bson::BSONElement &,
                         _mthSAction *,
                         bson::BSONElement & ) ;

   INT32 mthDivideBuild( const CHAR *,
                         const bson::BSONElement &,
                         _mthSAction *,
                         bson::BSONObjBuilder & ) ;

   INT32 mthDivideGet( const CHAR *,
                       const bson::BSONElement &,
                       _mthSAction *,
                       bson::BSONElement & ) ;

   INT32 mthSizeBuild( const CHAR *fieldName, const bson::BSONElement &e,
                       _mthSAction *action, bson::BSONObjBuilder &builder ) ;

   INT32 mthSizeGet( const CHAR *fieldName, const bson::BSONElement &in,
                     _mthSAction *action, bson::BSONElement &out ) ;

   INT32 mthTypeBuild( const CHAR *fieldName, const bson::BSONElement &e,
                       _mthSAction *action, bson::BSONObjBuilder &builder ) ;

   INT32 mthTypeGet( const CHAR *fieldName, const bson::BSONElement &in,
                     _mthSAction *action, bson::BSONElement &out ) ;

}

#endif


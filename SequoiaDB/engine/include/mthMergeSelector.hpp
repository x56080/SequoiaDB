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

   Source File Name = mthMergeSelector.hpp

   Descriptive Name = Method Merge Selector Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains structure for selecting
   operation, which is generating a new BSON record from a given record, based
   on the given selecting rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTHMERGESELECTOR_HPP_
#define MTHMERGESELECTOR_HPP_
#include "core.hpp"
#include "oss.hpp"
#include <vector>
#include "ossUtil.hpp"
#include "../bson/bson.h"
#include "../bson/bsonobj.h"
using namespace bson ;
namespace engine
{
   enum _MergeSelectorSide
   {
      MERGE_SELECTOR_SIDE_OUTER,
      MERGE_SELECTOR_SIDE_INNER,
      MERGE_SELECTOR_SIDE_UNKNOWN
   } ;
   class _MergeSelectorElement : public SDBObject
   {
   public :
      _MergeSelectorSide _side ;
      const CHAR *_pSourceName ;
      const CHAR *_pTargetName ;
      _MergeSelectorElement ()
      {
         _pSourceName = NULL ;
         _pTargetName = NULL ;
         _side = MERGE_SELECTOR_SIDE_UNKNOWN ;
      }
      INT32 init ( const CHAR *pSourceName,
                   const CHAR *pTargetName,
                   const CHAR *pOuterAlias,
                   const CHAR *pInnerAlias ) ;
   } ;
   typedef _MergeSelectorElement MergeSelectorElement ;
   class _mthMergeSelector : public SDBObject
   {
   private :
      const CHAR *_pOuterAlias ;
      const CHAR *_pInnerAlias ;
      BSONObj _selectorPattern ;
      BOOLEAN _initialized ;
      vector<MergeSelectorElement> _selectorElements ;
      INT32 _addSelector ( const BSONElement &ele ) ;

      INT32 _buildNewObj (
                           BSONObjBuilder &b,
                           const BSONObj &obj,
                           const CHAR *pSourceName,
                           const CHAR *pTargetName ) ;
   public :
      _mthMergeSelector ()
      {
         _initialized = FALSE ;
      }
      ~_mthMergeSelector()
      {
         _selectorElements.clear() ;
      }
      INT32 loadPattern ( const BSONObj &selectorPattern,
                          const CHAR *pOuterAlias,
                          const CHAR *pInnerAlias ) ;
      INT32 select ( const BSONObj &outer,
                     const BSONObj &inner,
                     BSONObj &target ) ;
      OSS_INLINE BOOLEAN isInitialized () { return _initialized ; }
      BSONObj getPattern ()
      {
         return _selectorPattern ;
      }
   } ;
   typedef _mthMergeSelector mthMergeSelector ;
}

#endif

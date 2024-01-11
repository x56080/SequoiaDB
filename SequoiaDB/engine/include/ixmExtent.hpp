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

   Source File Name = ixmExtent.hpp

   Descriptive Name = Index Management Extent Header

   When/how to use: this program may be used on binary and text-formatted
   versions of index management component. This file contains structure for
   index extent and its methods. The B Tree Insert/Delete/Update methods are
   also defined in this file.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IXMEXTENT_HPP_
#define IXMEXTENT_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "dms.hpp"
#include "ixm.hpp"
#include "ixmKey.hpp"
#include "pd.hpp"
#include "pmdEDU.hpp"
#include "dmsStorageBase.hpp"
#include "dmsPageMap.hpp"
#include "rtnPredicate.hpp"
#include "utilResult.hpp"

using namespace bson ;

namespace engine
{

   /*
      _ixmExtent define
   */
   class _ixmExtent : public SDBObject
   {
   public:
      // currentKey is the key from current disk location that trying to
      // be matched
      // prevKey is the key examed from previous run
      // keepFieldsNum is the number of fields from prevKey that should match
      // the
      // currentKey (for example if the prevKey is {c1:1, c2:1}, and
      // keepFieldsNum
      // = 1, that means we want to match c1:1 key for the current location.
      // Depends on if we have skipToNext set, if we do that means we want to
      // skip
      // c1:1 and match whatever the next (for example c1:1.1); otherwise we
      // want
      // to continue match the elements from matchEle )
      // Make this member function static so that other class can reuse the
      // key compare outside of the class or without an instance
      static INT32 keyCmp ( const BSONObj &currentKey, const BSONObj &prevKey,
                            INT32 keepFieldsNum, BOOLEAN skipToNext,
                            const VEC_ELE_CMP &matchEle,
                            const VEC_BOOLEAN &matchInclusive,
                            const Ordering &o, INT32 direction ) ;
   } ;
   typedef class _ixmExtent ixmExtent ;
}

#endif //IXMEXTENT_HPP_


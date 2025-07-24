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

   Source File Name = mthSelector.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTH_SELECTOR_HPP_
#define MTH_SELECTOR_HPP_

#include "mthSColumnMatrix.hpp"

namespace engine
{
   class _mthSelector : public SDBObject
   {
   public:
      _mthSelector() ;
      ~_mthSelector() ;

   public:
      OSS_INLINE BOOLEAN isInitialized() const
      {
         return _init ;
      }

      OSS_INLINE const bson::BSONObj getPattern() const
      {
         return _matrix.getPattern() ;
      }

      OSS_INLINE void setStringOutput ( BOOLEAN strOut )
      {
         _stringOutput = strOut ;
      }

      OSS_INLINE BOOLEAN getStringOutput () const
      {
         return _stringOutput ;
      }

   public:
      INT32 loadPattern( const bson::BSONObj &pattern, 
                         BOOLEAN strictDataMode = FALSE,
                         IXM_FIELD_NAME_SET *pSelectSet = NULL ) ;

      INT32 select( const bson::BSONObj &source,
                    bson::BSONObj &target ) ;

      INT32 move( _mthSelector &other ) ;

      void clear() ;

   private:
      INT32 _buildCSV( const bson::BSONObj &obj,
                       bson::BSONObj &csv ) ;

      INT32 _resortObj( const bson::BSONObj &pattern,
                        const bson::BSONObj &src,
                        bson::BSONObj &obj ) ;
   private:
      mthSColumnMatrix _matrix ;
      BOOLEAN _init ;

      ///csv
      BOOLEAN _stringOutput ;
      BOOLEAN _strictDataMode ;
      INT32 _stringOutputBufferSize ;
      CHAR *_stringOutputBuffer ;
   } ;
   typedef class _mthSelector mthSelector ;
}

#endif


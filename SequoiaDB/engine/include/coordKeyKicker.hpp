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

   Source File Name = coordKeyKicker.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          13/04/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_KEY_KICKER_HPP__
#define COORD_KEY_KICKER_HPP__

#include "coordResource.hpp"
#include "../bson/bson.h"
#include "ossMemPool.hpp"

using namespace bson ;

namespace engine
{

   /*
      _coordKeyKicker define
   */
   class _coordKeyKicker : public SDBObject
   {
      struct strContainner
      {
         const CHAR *_pStr ;
         strContainner( const CHAR *str )
         {
            _pStr = str ;
         }
         strContainner()
         {
            _pStr = NULL ;
         }
         BOOLEAN operator<( const strContainner &right ) const
         {
            if ( !right._pStr )
            {
               return FALSE ;
            }
            else if ( !_pStr )
            {
               return TRUE ;
            }
            return ossStrcmp( _pStr, right._pStr ) < 0 ? TRUE : FALSE ;
         }
      } ;

      typedef ossPoolSet< strContainner >  SET_KEEPKEY ;

   public:
      _coordKeyKicker() ;
      ~_coordKeyKicker() ;

      void     bind( coordResource *pResource,
                     const CoordCataInfoPtr &cataPtr ) ;

   public:
      INT32    kickKey( const BSONObj &updator,
                        BSONObj &newUpdator,
                        BOOLEAN &isChanged,
                        _pmdEDUCB *cb,
                        const BSONObj &matcher = BSONObj(),
                        BOOLEAN keepShardingKey = FALSE ) ;

      INT32    checkShardingKey( const BSONObj &updator,
                                 BOOLEAN &hasInclude,
                                 _pmdEDUCB *cb,
                                 const BSONObj &matcher = BSONObj() ) ;

   protected:
      BOOLEAN     _isUpdateReplace( const BSONObj &updator ) ;
      UINT32      _addKeys( const BSONObj &objKey ) ;

      INT32       _kickKey( const CoordCataInfoPtr &cataInfo,
                            const BSONObj &updator,
                            BSONObj &newUpdator,
                            const BSONObj &matcher,
                            BOOLEAN &shardingKeyChanged,
                            BOOLEAN &hasKeepAutoInc,
                            BOOLEAN ignoreAutoInc = FALSE ) ;

      INT32       _kickShardingKey( const string &collectionName,
                                    const BSONObj &updator,
                                    BSONObj &newUpdator,
                                    const BSONObj &matcher,
                                    BOOLEAN &isChanged,
                                    _pmdEDUCB *cb,
                                    BOOLEAN keepShardingKey ) ;

      INT32       _checkShardingKey( const CoordCataInfoPtr &cataInfo,
                                     const BSONObj &updator,
                                     BOOLEAN &hasInclude ) ;

      INT32       _checkShardingKey( const string &collectionName,
                                     const BSONObj &updator,
                                     BOOLEAN &hasInclude,
                                     _pmdEDUCB *cb ) ;

      BOOLEAN     _isKey( const CHAR *pField, BSONObj &boKey ) ;

      BOOLEAN     _isShardingKeyChange( const BSONElement &beField,
                                        const BSONObj &matcher ) ;

   private:
      typedef ossPoolMap< UINT32, BOOLEAN > SiteIDSet ;
      SiteIDSet                  _skSiteIDs ;
      SET_KEEPKEY                _setKeys ;

      coordResource              *_pResource ;
      CoordCataInfoPtr           _cataPtr ;

   } ;
   typedef _coordKeyKicker coordKeyKicker ;

}

#endif //COORD_KEY_KICKER_HPP__


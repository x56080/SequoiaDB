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

   Source File Name = dmsPageMap.hpp

   Descriptive Name = Data Management Service Header

   When/how to use: this program may be used on binary and text-formatted
   versions of data management component. This file contains structure for
   dms Reccord ID (RID).

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/26/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_PAGE_MAP_HPP__
#define DMS_PAGE_MAP_HPP__

#include "dms.hpp"
#include "ossAtomic.hpp"
#include <map>

namespace engine
{

   /*
      _dmsPageMap define
   */
   class _dmsPageMap : public SDBObject
   {
      friend class _dmsPageMapUnit ;

      public:
         typedef std::map< dmsExtentID, dmsExtentID >    MAP_PAGES ;
         typedef MAP_PAGES::iterator                     MAP_PAGES_IT ;
         typedef MAP_PAGES::const_iterator               MAP_PAGES_CIT ;

      public:
         _dmsPageMap() ;
         ~_dmsPageMap() ;

         BOOLEAN  isEmpty() ;
         UINT64   size() ;

         void  addItem( dmsExtentID src, dmsExtentID dst ) ;
         void  rmItem( dmsExtentID src ) ;
         void  clear() ;

         BOOLEAN findItem( dmsExtentID src, dmsExtentID *pDst ) const ;

         MAP_PAGES_IT begin() ;
         MAP_PAGES_IT end() ;
         void         erase( MAP_PAGES_IT pos ) ;

      private:
         void              _setInfo( ossAtomic64 *pTotalSize,
                                     ossAtomic32 *pNonEmptyNum ) ;

      private:
         MAP_PAGES         _mapPages ;
         ossAtomic64       _size ;
         ossAtomic64       *_pTotalSize ;
         ossAtomic32       *_pNonEmptyNum ;

   } ;
   typedef _dmsPageMap dmsPageMap ;

   /*
      _dmsPageMapUnit define
   */
   class _dmsPageMapUnit : public SDBObject
   {
      public:
         _dmsPageMapUnit() ;
         ~_dmsPageMapUnit() ;

         BOOLEAN        isEmpty() ;
         UINT64         totalSize() ;
         UINT32         nonEmptyNum() ;

         void           clear() ;

         dmsPageMap*    getMap( UINT16 slot ) ;

         dmsPageMap*    beginNonEmpty( UINT16 &slot ) ;
         dmsPageMap*    nextNonEmpty( UINT16 &slot ) ;

      private:
         dmsPageMap                 _mapSlot[ DMS_MME_SLOTS ] ;
         ossAtomic64                _totalSize ;
         ossAtomic32                _nonEmptyNum ;

   } ;
   typedef _dmsPageMapUnit dmsPageMapUnit ;

}

#endif /* DMS_PAGE_MAP_HPP__ */


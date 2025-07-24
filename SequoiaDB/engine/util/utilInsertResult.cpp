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

   Source File Name = utilInsertResult.cpp

   Descriptive Name = util insert error info

   When/how to use: N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/13/2019  LYB Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilInsertResult.hpp"
#include "msgDef.hpp"
#include "pd.hpp"

using namespace bson ;

namespace engine
{

   /*
      utilInsertResult implement
   */
   utilInsertResult::utilInsertResult()
   {
      enableMask( UTIL_RESULT_MASK_IDX ) ;
      _insertedNum = 0 ;
      _duplicatedNum = 0 ;
      _modifiedNum = 0 ;
      _enableReturnID = FALSE ;
   }

   utilInsertResult::~utilInsertResult()
   {
   }

   void utilInsertResult::_resetStat()
   {
      utilWriteResult::_resetStat() ;

      _returnIDObj = BSONObj() ;

      _insertedNum = 0 ;
      _duplicatedNum = 0 ;
      _modifiedNum = 0 ;
   }

   void utilInsertResult::_resetInfo()
   {
      utilWriteResult::_resetInfo() ;
   }

   void utilInsertResult::_toBSON( BSONObjBuilder &builder ) const
   {
      try
      {
         utilWriteResult::_toBSON( builder ) ;

         /// stat info
         builder.append( FIELD_NAME_INSERT_NUM, (INT64)_insertedNum ) ;
         builder.append( FIELD_NAME_DUPLICATE_NUM, (INT64)_duplicatedNum ) ;
         builder.append( FIELD_NAME_MODIFIED_NUM, (INT64)_modifiedNum ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Insert result to BSON occur exception: %s",
                 e.what() ) ;
      }
   }

   BOOLEAN utilInsertResult::_filterResultElement( const BSONElement &e ) const
   {
      if ( 0 == ossStrcmp( FIELD_NAME_INSERT_NUM, e.fieldName() ) ||
           0 == ossStrcmp( FIELD_NAME_DUPLICATE_NUM, e.fieldName() ) ||
           0 == ossStrcmp( FIELD_NAME_MODIFIED_NUM, e.fieldName() ) )
      {
         return FALSE ;
      }
      return utilWriteResult::_filterResultElement( e ) ;
   }

   void utilInsertResult::enableIndexErrInfo()
   {
      enableMask( UTIL_RESULT_MASK_IDX ) ;
   }

   void utilInsertResult::disableIndexErrInfo()
   {
      disableMask( UTIL_RESULT_MASK_IDX ) ;
   }

   BOOLEAN utilInsertResult::isEnaleIndexErrInfo() const
   {
      return isMaskEnabled( UTIL_RESULT_MASK_IDX ) ;
   }

   void utilInsertResult::enableReturnIDInfo()
   {
      _enableReturnID = TRUE ;
   }

   void utilInsertResult::disableReturnIDInfo()
   {
      _enableReturnID = FALSE ;
   }

   BOOLEAN utilInsertResult::isEnableReturnIDInfo() const
   {
      return _enableReturnID ;
   }

   void utilInsertResult::setReturnIDByObj( const BSONObj &obj )
   {
      if ( _enableReturnID )
      {
         try
         {
            BSONObjBuilder builder( 30 ) ;
            builder.append( obj.getField( DMS_ID_KEY_NAME ) ) ;
            _returnIDObj = builder.obj() ;
         }
         catch( std::exception &e )
         {
            PD_LOG( PDWARNING, "Occur exception: %s", e.what() ) ;
         }
      }
   }

   BSONObj utilInsertResult::getReturnIDObj() const
   {
      return _returnIDObj ;
   }

   /*
      utilUpdateResult implement
   */
   utilUpdateResult::utilUpdateResult()
   {
      _updatedNum = 0 ;
   }

   utilUpdateResult::~utilUpdateResult()
   {
   }

   void utilUpdateResult::_resetStat()
   {
      utilInsertResult::_resetStat() ;
      _updatedNum = 0 ;
   }

   void utilUpdateResult::_resetInfo()
   {
      utilInsertResult::_resetInfo() ;
      _currentFieldObj = BSONObj() ;
   }

   void utilUpdateResult::_toBSON( BSONObjBuilder &builder ) const
   {
      try
      {
         utilWriteResult::_toBSON( builder ) ;

         builder.append( FIELD_NAME_UPDATE_NUM, (INT64)_updatedNum ) ;
         builder.append( FIELD_NAME_MODIFIED_NUM, (INT64)_modifiedNum ) ;
         builder.append( FIELD_NAME_INSERT_NUM, (INT64)insertedNum() ) ;
         if ( !_currentFieldObj.isEmpty() )
         {
            builder.append( FIELD_NAME_CURRENT_FIELD, _currentFieldObj ) ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Update result to BSON occur exception: %s",
                 e.what() ) ;
      }
   }

   BOOLEAN utilUpdateResult::_filterResultElement( const BSONElement &e ) const
   {
      if ( 0 == ossStrcmp( FIELD_NAME_UPDATE_NUM, e.fieldName() ) ||
           0 == ossStrcmp( FIELD_NAME_MODIFIED_NUM, e.fieldName() ) ||
           0 == ossStrcmp( FIELD_NAME_INSERT_NUM, e.fieldName() ) )
      {
         return FALSE ;
      }
      return utilWriteResult::_filterResultElement( e ) ;
   }

   void utilUpdateResult::setCurrentField( BSONElement &currentEle )
   {
      try
      {
         if ( !currentEle.eoo() )
         {
            BSONObjBuilder b( 20 ) ;
            b.append( currentEle ) ;
            _currentFieldObj = b.obj() ;
         }
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Set current field occur exception: %s", e.what() ) ;
      }
   }

   /*
      utilDeleteResult implement
   */
   utilDeleteResult::utilDeleteResult()
   {
      disableMask( UTIL_RESULT_MASK_IDX ) ;
      _deletedNum = 0 ;
   }

   utilDeleteResult::~utilDeleteResult()
   {
   }

   void utilDeleteResult::_resetStat()
   {
      utilWriteResult::_resetStat() ;
      _deletedNum = 0 ;
   }

   void utilDeleteResult::_resetInfo()
   {
      utilWriteResult::_resetInfo() ;
   }

   void utilDeleteResult::_toBSON( BSONObjBuilder &builder ) const
   {
      try
      {
         utilWriteResult::_toBSON( builder ) ;

         builder.append( FIELD_NAME_DELETE_NUM, (INT64)_deletedNum ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Delete result to BSON occur exception: %s",
                 e.what() ) ;
      }
   }

   BOOLEAN utilDeleteResult::_filterResultElement( const BSONElement &e ) const
   {
      if ( 0 == ossStrcmp( FIELD_NAME_DELETE_NUM, e.fieldName() ) )
      {
         return FALSE ;
      }
      return utilWriteResult::_filterResultElement( e ) ;
   }

}



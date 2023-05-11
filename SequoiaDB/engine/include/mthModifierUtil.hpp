/*******************************************************************************


   Copyright (C) 2011-2023 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = mthModifierUtil.hpp

   Descriptive Name = Method Modifier Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains structure for modify
   operation, which is changing a data record based on modification rule.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/11/2023  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTHMODIFIER_UTIL_HPP_
#define MTHMODIFIER_UTIL_HPP_

#include "mthModifierDef.hpp"
#include <vector>
#include "ossUtil.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "../bson/bson.h"
#include "mthCommon.hpp"

using namespace bson ;

namespace engine
{

   #define MTH_DOLLAR_FIELD_SIZE          (10)
   /*
      _mthModCompareNatrualOrder define
   */
   class _mthModCompareNatrualOrder
   {
   private:
      vector<INT64> *_dollarList ;
      CHAR           _dollarBuff[ MTH_DOLLAR_FIELD_SIZE + 1 ] ;

   private:
      OSS_INLINE BOOLEAN _isNumber( CHAR c )
      {
         return c >= '0' && c <= '9' ;
      }

      INT32 _lexNumCmp ( const CHAR *s1, const CHAR *s2 ) ;

   public:
      _mthModCompareNatrualOrder( vector<INT64> *dollarList = NULL )
      : _dollarList(NULL)
      {
         _dollarList = dollarList ;
         ossMemset( _dollarBuff, 0, sizeof(_dollarBuff) ) ;
      }

      void setDollarList( vector<INT64> *dollarList )
      {
         _dollarList = dollarList ;
      }

      const CHAR *getDollarValue ( const CHAR *s,
                                   BOOLEAN *pUnknowDollar = NULL ) ;

      FieldCompareResult compField( const char* l, const char* r,
                                    UINT32 *pLeftPos = NULL,
                                    UINT32 *pRightPos = NULL ) ;

   } ;
   typedef _mthModCompareNatrualOrder mthModCompareNatrualOrder ;

   /*
      _mthModifierOpMap define
   */
   class _mthModifierOpMap : public SDBObject
   {
      public:
         _mthModifierOpMap() ;
         ~_mthModifierOpMap() {}

         ModType find( const CHAR* modifierName ) const ;

      private:
         typedef map< const CHAR*, INT32, mthStrcasecmp > MODIFIER_MAP ;
         MODIFIER_MAP _modifiersMap ;
   } ;
   typedef _mthModifierOpMap mthModifierOpMap ;

   const mthModifierOpMap& mthGetModifierOpParse() ;

   /*
      Common functions
   */
   BOOLEAN mthIsBiggerNumberType( const BSONElement &left,
                                  const BSONElement &right ) ;

   /**
    * @brief Evaluate the fields to be updated by '$inc' operator, and put them
    *        into the builder for the final record.
    * @param existElement   The current field element to be modified.
    * @param inc            '$inc' value for the field to be modified.
    * @param strictMode     Whether overflow is allowed in calculation.
    * @param resBuilder     Builder for the final record.
    */
   INT32 mthModifierInc( const BSONElement& existElement,
                         const BSONElement &inc, BOOLEAN strictMode,
                         BSONObjBuilder &resBuilder ) ;


}

#endif //MTHMODIFIER_UTIL_HPP_


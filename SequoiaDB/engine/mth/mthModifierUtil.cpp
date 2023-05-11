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

   Source File Name = mthModifierUtil.cpp

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

#include "mthModifierUtil.hpp"
#include "utilMath.hpp"

using namespace bson ;

namespace engine
{

   /*
      _mthModCompareNatrualOrder implement
   */
   const CHAR *_mthModCompareNatrualOrder::getDollarValue ( const CHAR *s,
                                                            BOOLEAN *pUnknowDollar )
   {
      const CHAR *retStr = NULL ;

      if ( pUnknowDollar )
      {
         *pUnknowDollar = FALSE ;
      }

      if ( _dollarList && *s && '$' == *s )
      {
         INT64 temp = 0 ;
         INT32 dollarNum = 0 ;
         INT32 num = -1 ;

         if ( SDB_OK != ossStrToInt ( s + 1, &dollarNum ) )
         {
            goto done ;
         }

         for ( vector<INT64>::iterator it = _dollarList->begin() ;
               it != _dollarList->end() ; ++it )
         {
            temp = *it ;
            if ( dollarNum == ((temp>>32)&0xFFFFFFFF) )
            {
               num = (temp&0xFFFFFFFF) ;
               ossMemset ( _dollarBuff, 0, sizeof(_dollarBuff) ) ;
               ossSnprintf ( _dollarBuff, MTH_DOLLAR_FIELD_SIZE, "%d", num ) ;
               retStr = &_dollarBuff[0] ;
               goto done ;
            }
         }

         if ( pUnknowDollar )
         {
            *pUnknowDollar = TRUE ;
         }
      }

   done:
      return retStr ? retStr : s ;
   }

   INT32 _mthModCompareNatrualOrder::_lexNumCmp ( const CHAR *s1, const CHAR *s2 )
   {
      BOOLEAN p1 = FALSE ;
      BOOLEAN p2 = FALSE ;
      BOOLEAN n1 = FALSE ;
      BOOLEAN n2 = FALSE ;

      const CHAR   *e1 = NULL ;
      const CHAR   *e2 = NULL ;

      INT32   len1   = 0 ;
      INT32   len2   = 0 ;
      INT32   result = 0 ;

      CHAR t1[MTH_DOLLAR_FIELD_SIZE+1] = {0} ;
      CHAR t2[MTH_DOLLAR_FIELD_SIZE+1] = {0} ;

      if ( _dollarList && *s1 && '$' == *s1 )
      {
         INT64 temp = 0 ;
         INT32 dollarNum = 0 ;
         INT32 num = -1 ;

         if ( SDB_OK == ossStrToInt ( s1 + 1, &dollarNum ) )
         {
            for ( vector<INT64>::iterator it = _dollarList->begin() ;
                  it != _dollarList->end() ; ++it )
            {
               temp = *it ;
               if ( dollarNum == ((temp>>32)&0xFFFFFFFF) )
               {
                  num = (temp&0xFFFFFFFF) ;
                  ossSnprintf ( t1, MTH_DOLLAR_FIELD_SIZE, "%d", num ) ;
                  s1 = t1 ;
                  break ;
               }
            }
         }
      }

      if ( _dollarList && *s2 && '$' == *s2 )
      {
         INT64 temp = 0 ;
         INT32 dollarNum = 0 ;
         INT32 num = -1 ;

         if ( SDB_OK == ossStrToInt ( s2 + 1, &dollarNum ) )
         {
            for ( vector<INT64>::iterator it = _dollarList->begin() ;
                  it != _dollarList->end() ; ++it )
            {
               temp = *it ;
               if ( dollarNum == ((temp>>32)&0xFFFFFFFF) )
               {
                  num = (temp&0xFFFFFFFF) ;
                  ossSnprintf ( t2, MTH_DOLLAR_FIELD_SIZE, "%d", num ) ;
                  s2 = t2 ;
                  break ;
               }
            }
         }
      }

      while( *s1 && *s2 )
      {
         p1 = ( *s1 == (CHAR)255 ) ;
         p2 = ( *s2 == (CHAR)255 ) ;
         if ( p1 && !p2 )
         {
            return 1 ;
         }
         else if ( !p1 && p2 )
         {
            return -1 ;
         }

         n1 = _isNumber( *s1 ) ;
         n2 = _isNumber( *s2 ) ;

         if ( n1 && n2 )
         {
            INT32 zerolen1 = 0 ;
            INT32 zerolen2 = 0 ;
            // get rid of leading 0s
            while ( *s1 == '0' )
            {
               ++s1 ;
               ++zerolen1 ;
            }
            while ( *s2 == '0' )
            {
               ++s2 ;
               ++zerolen2 ;
            }

            e1 = s1 ;
            e2 = s2 ;
            // find length
            // if end of string, will break immediately ('\0')
            while ( _isNumber ( *e1 ) )
            {
               ++e1 ;
            }
            while ( _isNumber ( *e2 ) )
            {
               ++e2 ;
            }

            len1 = (INT32)( e1 - s1 ) ;
            len2 = (INT32)( e2 - s2 ) ;

            // if one is longer than the other, return
            if ( len1 > len2 )
            {
               return 1 ;
            }
            else if ( len2 > len1 )
            {
               return -1 ;
            }
            // if the lengths are equal, just strcmp
            else if ( ( result = ossStrncmp ( s1, s2, len1 ) ) != 0 )
            {
               return result ;
            }
            else if ( zerolen1 != zerolen2 )
            {
               return zerolen1 < zerolen2 ? 1 : -1 ;
            }
            // otherwise, the numbers are equal
            s1 = e1 ;
            s2 = e2 ;
            continue ;
         }

         if ( n1 )
         {
            return 1 ;
         }
         else if ( n2 )
         {
            return -1 ;
         }
         else if ( *s1 > *s2 )
         {
            return 1 ;
         }
         else if ( *s2 > *s1 )
         {
            return -1 ;
         }

         ++s1 ;
         ++s2 ;
      }

      if ( *s1 )
      {
         return 1 ;
      }
      else if ( *s2 )
      {
         return -1 ;
      }
      return 0 ;
   }

   FieldCompareResult _mthModCompareNatrualOrder::compField ( const CHAR* l,
                                                              const CHAR* r,
                                                              UINT32 *pLeftPos,
                                                              UINT32 *pRightPos )
   {
      const CHAR *pLTmp = l ;
      const CHAR *pRTmp = r ;
      const CHAR *pLDot = NULL ;
      const CHAR *pRDot = NULL ;
      INT32 result = 0 ;

      MTH_SUBFIELD_STR   lStr ;
      MTH_SUBFIELD_STR   rStr ;

      if ( pLeftPos )
      {
         *pLeftPos = 0 ;
      }
      if ( pRightPos )
      {
         *pRightPos = 0 ;
      }

      while ( pLTmp && pRTmp && *pLTmp && *pRTmp )
      {
         pLDot = ossStrchr( pLTmp, '.' ) ;
         pRDot = ossStrchr( pRTmp, '.' ) ;

         if ( pLDot )
         {
            lStr.append( pLTmp, pLDot - pLTmp ) ;
            pLTmp = lStr.str() ;
         }
         if ( pRDot )
         {
            rStr.append( pRTmp, pRDot - pRTmp ) ;
            pRTmp = rStr.str() ;
         }
         result = _lexNumCmp( pLTmp, pRTmp ) ;
         // clear
         if ( pLDot )
         {
            lStr.clear() ;
         }
         if ( pRDot )
         {
            rStr.clear() ;
         }

         if ( result < 0 )
         {
            return LEFT_BEFORE ;
         }
         else if ( result > 0 )
         {
            return RIGHT_BEFORE ;
         }

         // SAME
         pLTmp = pLDot ? pLDot + 1 : NULL ;
         pRTmp = pRDot ? pRDot + 1 : NULL ;

         if ( pLeftPos )
         {
            *pLeftPos = pLTmp ? pLTmp - l : ossStrlen( l ) ;
         }
         if ( pRightPos )
         {
            *pRightPos = pRTmp ? pRTmp - r : ossStrlen( r ) ;
         }
      }

      if ( !pLTmp || !*pLTmp )
      {
         if ( !pRTmp || !*pRTmp )
         {
            return SAME ;
         }
         return RIGHT_SUBFIELD ;
      }
      return LEFT_SUBFIELD ;
   }

   /*
      _mthModifierOpMap implement
   */
   _mthModifierOpMap::_mthModifierOpMap()
   {
      _modifiersMap[ MTH_MODIFIER_INC ] = INC ;
      _modifiersMap[ MTH_MODIFIER_SET ] = SET ;
      _modifiersMap[ MTH_MODIFIER_PUSH ] = PUSH ;
      _modifiersMap[ MTH_MODIFIER_PUSH_ALL ] = PUSH_ALL ;
      _modifiersMap[ MTH_MODIFIER_PULL ] = PULL ;
      _modifiersMap[ MTH_MODIFIER_PULL_BY ] = PULL_BY ;
      _modifiersMap[ MTH_MODIFIER_PULL_ALL ] = PULL_ALL ;
      _modifiersMap[ MTH_MODIFIER_PULL_ALL_BY ] = PULL_ALL_BY ;
      _modifiersMap[ MTH_MODIFIER_PULL_ALL_BY ] = PULL_ALL_BY ;
      _modifiersMap[ MTH_MODIFIER_POP ] =  POP ;
      _modifiersMap[ MTH_MODIFIER_UNSET ] =  UNSET ;
      _modifiersMap[ MTH_MODIFIER_BITNOT ] = BITNOT ;
      _modifiersMap[ MTH_MODIFIER_BITXOR ] = BITXOR ;
      _modifiersMap[ MTH_MODIFIER_BITAND ] = BITAND ;
      _modifiersMap[ MTH_MODIFIER_BITOR ] = BITOR ;
      _modifiersMap[ MTH_MODIFIER_BIT ] = BIT ;
      _modifiersMap[ MTH_MODIFIER_ADDTOSET ] = ADDTOSET ;
      _modifiersMap[ MTH_MODIFIER_RENAME ] = RENAME ;
      _modifiersMap[ MTH_MODIFIER_NULLOPR ] = NULLOPR ;
      _modifiersMap[ MTH_MODIFIER_REPLACE ] = REPLACE ;
      _modifiersMap[ MTH_MODIFIER_KEEP ] = KEEP ;
      _modifiersMap[ MTH_MODIFIER_SETARRAY ] = SETARRAY ;
      _modifiersMap[ MTH_MODIFIER_CURRENTDATE ] = CURRENT_DATE ;
   }

   ModType _mthModifierOpMap::find( const CHAR * modifierName ) const
   {
      SDB_ASSERT ( modifierName, "modifierName can't be NULL " ) ;

      MODIFIER_MAP::const_iterator citr ;
      citr = _modifiersMap.find( modifierName ) ;
      if ( citr != _modifiersMap.end() )
      {
         return (ModType)citr->second ;
      }
      else
      {
         return UNKNOWN ;
      }
   }

   const mthModifierOpMap& mthGetModifierOpParse()
   {
      static _mthModifierOpMap s_modifierOpMap ;
      return s_modifierOpMap ;
   }

   /*
      Common functions implement
   */
   BOOLEAN mthIsBiggerNumberType( const BSONElement &left,
                                  const BSONElement &right )
   {
      if ( left.isNumber() && right.isNumber() )
      {
         if ( left.type() == NumberDecimal && right.type() != NumberDecimal )
         {
            return TRUE ;
         }
         else if ( left.type() == NumberDouble
                   && (right.type() == NumberInt || right.type() == NumberLong))
         {
            return TRUE ;
         }
         else if ( left.type() == NumberLong && right.type() == NumberInt )
         {
            return TRUE ;
         }

         return FALSE ;
      }

      return left.type() > right.type() ;
   }

   INT32 mthModifierInc( const BSONElement &existElement,
                         const BSONElement &incElement,BOOLEAN strictMode,
                         BSONObjBuilder &resBuilder )
   {
      INT32 rc = SDB_OK ;
      BSONType existType = existElement.type() ;
      BSONType incType = incElement.type() ;

      if ( existElement.isNumber() && ( NumberDecimal == existType ||
                                        NumberDecimal == incType ) )
      {
         bsonDecimal inc ;
         bsonDecimal decimal ;

         decimal = existElement.numberDecimal() ;
         inc     = incElement.numberDecimal() ;
         if ( inc.isZero() )
         {
            // empty resBuilder imply existElement is not changed
         }
         else
         {
            bsonDecimal result ;
            rc = decimal.add( inc, result ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "decimal add failed:v1=%s,v2=%s,rc=%d",
                           decimal.toString().c_str(),
                           inc.toString().c_str(), rc ) ;
               goto error ;
            }

            rc = result.updateTypemod( decimal.getTypemod() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG_MSG( PDERROR, "result is out of precision:result=%s,"
                           "precision=%d,scale=%d,rc=%d",
                           result.toString().c_str(),
                           decimal.getPrecision(), decimal.getScale(), rc ) ;
               goto error ;
            }
            resBuilder.append( "", result ) ;
         }
      }
      else if ( existElement.isNumber() && 0 != incElement.numberDouble() )
      {
         if ( NumberDouble == existType || NumberDouble == incType )
         {
            resBuilder.append( "", existElement.numberDouble()
                                   + incElement.numberDouble() ) ;
         }
         else if ( NumberLong == existType || NumberLong == incType )
         {
            INT64 arg1 = existElement.numberLong() ;
            INT64 arg2 = incElement.numberLong() ;
            INT64 result = arg1 + arg2 ;

            if ( !utilAddIsOverflow( arg1, arg2, result) )
            {
               if ( NumberInt == existType && utilCanConvertToINT32( result ) )
               {
                  // keep int if possible
                  resBuilder.append( "", (INT32)result ) ;
               }
               else
               {
                  resBuilder.append( "", result ) ;
               }
            }
            else if ( !strictMode )
            {
               // overflow
               bsonDecimal decimalE ;
               bsonDecimal decimalArg ;
               bsonDecimal decimalResult ;

               decimalE   = existElement.numberDecimal() ;
               decimalArg = incElement.numberDecimal() ;
               rc = decimalE.add( decimalArg, decimalResult ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to add decimal:%s+%s,rc=%d",
                          decimalE.toString().c_str(),
                          decimalArg.toString().c_str(), rc ) ;
                  goto error ;
               }

               resBuilder.append( "", decimalResult ) ;
            }
            else
            {
               rc = SDB_VALUE_OVERFLOW ;
               PD_LOG( PDERROR, "overflow happened, field: %s(%lld, inc: %lld),"
                       " rc = %d", existElement.fieldName(), arg1, arg2, rc ) ;
               goto error ;
            }
         }
         else
         {
            INT32 arg1 = existElement.numberInt();
            INT32 arg2 = incElement.numberInt() ;

            INT32 result = arg1 + arg2 ;
            INT64 result64 = (INT64)arg1 + (INT64)arg2 ;
            if ( result64 == (INT64)result )
            {
               resBuilder.append( "", result ) ;
            }
            else if ( !strictMode )
            {
               resBuilder.append( "", result64 ) ;
            }
            else
            {
               //32 bit overflow or underflow happened
               rc = SDB_VALUE_OVERFLOW ;
               PD_LOG( PDERROR, "overflow happened, field: %s(%d, inc: %d), "
                       "rc = %d", existElement.fieldName(), arg1, arg2, rc ) ;
               goto error ;
            }
         }
      }
      else if ( existElement.eoo() && incElement.isNumber() )
      {
         resBuilder.append( incElement ) ;
      }
      else
      {
         // empty resBuilder imply existElement is not changed
      }

   done:
      return rc ;
   error:
      goto done ;
   }


}



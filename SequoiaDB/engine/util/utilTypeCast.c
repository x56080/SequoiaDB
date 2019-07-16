#include "utilTypeCast.h"
#include <math.h>

//Note: The last bit of the boundary value is not included here.
#define UTIL_NUM_INT32_MAX_THRESHOLD 214748364
#define UTIL_NUM_INT32_MIN_THRESHOLD (-214748364)
#define UTIL_NUM_INT64_MAX_THRESHOLD 922337203685477580
#define UTIL_NUM_INT64_MIN_THRESHOLD (-922337203685477580)

#define UTIL_NUM_TYPE_INT32   0
#define UTIL_NUM_TYPE_INT64   1
#define UTIL_NUM_TYPE_FLOAT64 2
#define UTIL_NUM_TYPE_DECIMAL 3

#define DOUBLE_PRECISION   (15)
#define DOUBLE_MAX_EXP     (308)
#define DOUBLE_BOUND       (1.79)

static FLOAT64 powersOf10[] = {
   10.,
   100.,
   1.0e4,
   1.0e8,
   1.0e16,
   1.0e32,
   1.0e64,
   1.0e128,
   1.0e256
} ;

/*
 * \brief Convert a string to a numeric value
 *        Note: 1. [+/-]inf, [+/-]Infinity and nan are not supported.
 *              2. If type is 1, it means decimal type, but it does not
 *                 support decimal type, it needs to be processed by itself.
 *
 * \param [in]  data          String pointer to be parsed
 * \param [in]  length        Reserved, not used now
 * \param [out] type          Type of value:
 *                               0: INT32
 *                               1: INT64
 *                               2: FLOAT64
 *                               3: Decimal
 * \param [out] value         Numeric value
 * \param [out] valueLength   The length of the value
 * \retval SDB_OK Retrieval Success
 */
SDB_EXPORT INT32 utilStrToNumber( const CHAR* data, INT32 length,
                                  INT32 *type, utilNumberVal *value,
                                  INT32 *valueLength )
{
   INT32 rc       = SDB_OK ;
   INT32 len      = 0 ;
   INT32 sign     = 1 ;
   INT32 n1       = 0 ;
   INT32 digit    = -1 ;
   INT32 numType  = 0 ;
   INT32 fracExp  = 0 ;
   INT32 subscale = 0 ;
   INT32 signsubscale = 1 ;
   INT64 n2  = 0 ;
   FLOAT64 n = 0 ;
   const CHAR *pStr = data ;

   //step 1
   if ( *pStr == '#' )
   {
      //#xxx
      ++pStr ;
      ++len ;
   }

   if ( *pStr == '-' )
   {
      //-xxx
      sign = -1 ;
      ++pStr ;
      ++len ;
   }
   else if ( *pStr == '+' )
   {
      //+xxx
      sign = 1 ;
      ++pStr ;
      ++len ;
   }

   //step 2
   while ( *pStr == '0' )
   {
      //0xxxxx
      ++pStr ;
      ++len ;
   }

   //step 3
   {
      INT32 decPt = -1; 
      INT32 mantSize = 0 ;
      INT32 frac1Len = 0 ;
      INT32 frac1 = 0 ;
      INT32 frac2 = 0 ;
      INT32 invalidDecimal = 0 ;
      INT32 crossDecimal = 0 ;

      while ( *pStr >= '0' && *pStr <= '9' )
      {
         //<number>xxxx
         INT32 num = *pStr - '0' ;

         if ( numType == UTIL_NUM_TYPE_INT32 )
         {
            if ( n1 > UTIL_NUM_INT32_MAX_THRESHOLD )
            {
               //n1 * 10 is greater than the int range
               numType = UTIL_NUM_TYPE_INT64 ;
            }
            else if ( n1 < UTIL_NUM_INT32_MIN_THRESHOLD )
            {
               //n1 * 10 is less than the int range
               numType = UTIL_NUM_TYPE_INT64 ;
            }
            else if ( n1 == UTIL_NUM_INT32_MAX_THRESHOLD ||
                      n1 == UTIL_NUM_INT32_MIN_THRESHOLD )
            {
               if ( sign == 1 && num > 7 )
               {
                  //n1 * 10 + num is greater than the max int
                  numType = UTIL_NUM_TYPE_INT64 ;
               }
               else if ( sign == -1 && num > 8 )
               {
                  //n1 * 10 - num is less then the min int
                  numType = UTIL_NUM_TYPE_INT64 ;
               }
            }
         }
         else if ( numType == UTIL_NUM_TYPE_INT64 )
         {
            if ( n2 > UTIL_NUM_INT64_MAX_THRESHOLD )
            {
               //n2 * 10 is greater than the long long range
               numType = UTIL_NUM_TYPE_DECIMAL ;
               ++pStr ;
               ++len ;
               goto done ;
            }
            else if ( n2 < UTIL_NUM_INT64_MIN_THRESHOLD )
            {
               //n2 * 10 is less than the long long range
               numType = UTIL_NUM_TYPE_DECIMAL ;
               ++pStr ;
               ++len ;
               goto done ;
            }
            else if ( n2 == UTIL_NUM_INT64_MAX_THRESHOLD ||
                      n2 == UTIL_NUM_INT64_MIN_THRESHOLD )
            {
               if ( sign == 1 && num > 7 )
               {
                  //n2 * 10 + num is greater than the max long long
                  numType = UTIL_NUM_TYPE_DECIMAL ;
                  ++pStr ;
                  ++len ;
                  goto done ;
               }
               else if ( sign == -1 && num > 8 )
               {
                  //n2 * 10 - num is less then the min long long
                  numType = UTIL_NUM_TYPE_DECIMAL ;
                  ++pStr ;
                  ++len ;
                  goto done ;
               }
            }
         }

         if ( digit >= 0 || num > 0 )
         {
            ++digit ;
         }

         if ( digit >= 18 )
         {
         }
         else if ( digit < 9 )
         {
            frac1 = 10 * frac1 + num ;
         }
         else
         {
            frac2 = 10 * frac2 + num ;
            ++frac1Len ;
         }

         n1 = ( n1 * 10 ) + sign * num ;
         n2 = ( n2 * 10 ) + sign * num ;

         ++pStr ;
         ++len ;
         ++mantSize ;
         decPt = mantSize ;
      }

      //step 4
      if ( *pStr == '.' )
      {
         ++pStr ;
         ++len ;

         if( *pStr >= '0' && *pStr <= '9' )
         {
            //<number>.xxx
            BOOLEAN isSkipFrac2 = FALSE ;
            invalidDecimal = 0 ;
            numType = UTIL_NUM_TYPE_FLOAT64 ;

            if ( decPt < 0 && mantSize == 0 )
            {
               decPt = 1 ;
               mantSize = 1 ;
            }

            do
            {
               INT32 num = *pStr - '0' ;

               if ( num == 0 )
               {
                  ++invalidDecimal ;
               }
               else
               {
                  invalidDecimal = 0 ;
               }

               if ( digit >= 0 || num > 0 )
               {
                  ++digit ;
               }

               if ( digit >= 18 )
               {
                  if ( num == 0 )
                  {
                     ++crossDecimal ;
                  }
                  else
                  {
                     crossDecimal = 0 ;
                  }
               }
               else if ( digit < 9 )
               {
                  frac1 = 10 * frac1 + num ;
               }
               else
               {
                  frac2 = 10 * frac2 + num ;
                  ++frac1Len ;

                  if ( num == 0 && isSkipFrac2 == FALSE )
                  {
                     ++crossDecimal ;
                  }
                  else
                  {
                     crossDecimal = 0 ;
                     isSkipFrac2 = TRUE ;
                  }
               }

               ++pStr ;
               ++len ;
               ++mantSize ;
            } while ( *pStr >= '0' && *pStr <= '9' ) ;
         }
         else
         {
            goto finish ;
         }
      }

      mantSize -= crossDecimal ;
      digit -= invalidDecimal ;

      if ( digit >= 9 )
      {
         n = ( pow( 10.0, frac1Len ) * frac1 ) + frac2 ;
      }
      else
      {
         n = frac1 ;
      }

      if( numType == UTIL_NUM_TYPE_FLOAT64 && digit >= DOUBLE_PRECISION )
      {
         /*
          * The effective number is greater than 15 digits
          */
         numType = UTIL_NUM_TYPE_DECIMAL ;
         goto done ;
      }

      fracExp = decPt - mantSize ;
   }

   if ( pStr == data )
   {
      // no digits
      rc = SDB_INVALIDARG;
      goto error;
   }

   //step 5
   if ( *pStr == 'e' || *pStr == 'E' )
   {
      numType = UTIL_NUM_TYPE_FLOAT64 ;
      //<number>[e/E]xxx
      ++pStr ;
      ++len ;

      if ( *pStr == '+' )
      {
         ++pStr ;
         ++len ;
      }
      else if ( *pStr == '-' )
      {
         signsubscale = -1 ;
         ++pStr ;
         ++len ;
      }

      while ( *pStr >= '0' && *pStr <= '9' )
      {
         subscale = ( subscale * 10 ) + ( *pStr - '0' ) ;
         ++pStr ;
         ++len ;
      }
   }

   //step 6
   if ( numType == UTIL_NUM_TYPE_FLOAT64 )
   {
      FLOAT64 dblExp = 1.0 ;
      FLOAT64 *d = NULL ;

      if ( signsubscale == -1 )
      {
         subscale = fracExp - subscale ;
      }
      else
      {
         subscale = fracExp + subscale ;
      }

      if ( subscale < 0 )
      {
         signsubscale = -1 ;
         subscale = -subscale ;
      }
      else
      {
         signsubscale = 1 ;
      }

      if ( signsubscale == 1 && digit + subscale > DOUBLE_MAX_EXP )
      {
         /*
          * Maximum index should not exceed 308
          */
         numType = UTIL_NUM_TYPE_DECIMAL ;
         goto done ;
      }
      else if ( signsubscale == -1 && subscale - digit > DOUBLE_MAX_EXP )
      {
         /*
          * Minimum index should not exceed -308
          */
         numType = UTIL_NUM_TYPE_DECIMAL ;
         goto done ;
      }

      if( ( subscale > DOUBLE_MAX_EXP ) ||
          ( signsubscale == 1 && digit + subscale == DOUBLE_MAX_EXP ) ||
          ( signsubscale == -1 && subscale - digit == DOUBLE_MAX_EXP ) )
      {
         INT32 tmpFracExp = digit ;
         FLOAT64 tmpN = n ;
         FLOAT64 tmpDblExp = 1.0 ;

         for ( d = powersOf10; tmpFracExp > 0; tmpFracExp >>= 1, ++d )
         {
            if ( tmpFracExp & 01 )
            {
               tmpDblExp *= *d ;
            }
         }

         tmpN /= tmpDblExp ;

         if ( signsubscale == 1 && digit + subscale == DOUBLE_MAX_EXP &&
              tmpN > DOUBLE_BOUND )
         {
            /*
             * The maximum value of floating point number
             * should not exceed +/-1.79E+308
             */
            numType = UTIL_NUM_TYPE_DECIMAL ;
            goto done ;
         }
         else if ( signsubscale == -1 && subscale - digit == DOUBLE_MAX_EXP &&
                   tmpN > DOUBLE_BOUND )
         {
            /*
             * The minimum value of floating point number
             * should not exceed +/-1.79E-308
             */
            numType = UTIL_NUM_TYPE_DECIMAL ;
            goto done ;
         }

         if ( subscale > DOUBLE_MAX_EXP )
         {
            n = tmpN ;

            if ( signsubscale == 1 )
            {
               subscale += digit ;
            }
            else
            {
               subscale -= digit ;
               if ( subscale < 0 )
               {
                  signsubscale = -signsubscale ;
                  subscale = -subscale ;
               }
            }
         }
      }

      for ( d = powersOf10; subscale > 0; subscale >>= 1, ++d )
      {
         if ( subscale & 01 )
         {
            dblExp *= *d ;
         }
      }

      if ( signsubscale == 1 )
      {
         n *= dblExp ;
      }
      else
      {
         n /= dblExp ;
      }

      if ( sign < 0 )
      {
         n = -n ;
      }
   }

finish:
   if( value )
   {
      if ( numType == UTIL_NUM_TYPE_FLOAT64 )
      {
         value->doubleVal = n ;
      }
      else if( numType == UTIL_NUM_TYPE_INT32 )
      {
         value->intVal = n1 ;
      }
      else if( numType == UTIL_NUM_TYPE_INT64 )
      {
         value->longVal = n2 ;
      }
   }

done:
   if( type )
   {
      *type = numType ;
   }

   if ( valueLength )
   {
      *valueLength = len ;
   }
   return rc ;
error:
   goto done ;
}
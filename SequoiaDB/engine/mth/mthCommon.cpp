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

   Source File Name = mthCommon.cpp

   Descriptive Name = Method Common

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component. This file contains common functions for mth

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/12/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "ossTypes.h"
#include "mthCommon.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "mthTrace.hpp"
#include "mthDef.hpp"
#include "../util/fromjson.hpp"
#include "utilMath.hpp"
#include "ossMemPool.hpp"
#include "utilLocale.hpp"


using namespace bson ;

namespace engine
{
   #define MTH_OPERATOR_STR_AND  "$and"
   #define MTH_OPERATOR_STR_OR   "$or"
   #define MTH_OPERATOR_STR_NOT  "$not"

   #define MTH_INT64_SAFE_UP_BOUND   (4999999999999999999)
   #define MTH_INT64_SAFE_LOW_BOUND  (-4999999999999999999)
   #define MTH_FORMAT_MAX_SCALE      (SDB_DECIMAL_DSCALE_MASK)

   struct mthCastStr2Type
   {
      CHAR *castStr ;
      BSONType castType ;
   } ;

   static mthCastStr2Type g_cast_str_to_type_array[] =
   {
      //castStr,                      castType
      { "minkey",                     MinKey },
      { "double",                     NumberDouble },
      { "string",                     String },
      { "object",                     Object },
      { "array",                      Array },
      { "bindata",                    BinData },
      { "oid",                        jstOID },
      { "bool",                       Bool },
      { "date",                       Date },
      { "null",                       jstNULL },
      { "int32",                      NumberInt },
      { "timestamp",                  Timestamp },
      { "int64",                      NumberLong },
      { "regex",                      RegEx },
      { "decimal",                    NumberDecimal },
      { "maxkey",                     MaxKey },
   } ;

   static INT32 _mthAbsBasic( const CHAR *name, const BSONElement &in,
                              BSONObjBuilder &outBuilder, INT32 &flag ) ;
   static INT32 _mthCeilingBasic( const CHAR *name, const BSONElement &in,
                                  BSONObjBuilder &outBuilder ) ;
   static INT32 _mthFloorBasic( const CHAR *name, const BSONElement &in,
                                BSONObjBuilder &outBuilder ) ;
   static INT32 _mthRoundBasic( const CHAR * name, const BSONElement & in,
                                INT32 scale, INT32 &flag,
                                BSONObjBuilder & outBuilder ) ;
   static INT32 _mthFormatBasic( const CHAR * name, const BSONElement & in,
                                 INT32 scale, BSONObjBuilder & outBuilder ) ;
   static INT32 _mthModBasic( const CHAR *name, const BSONElement &in,
                              const BSONElement &modm,
                              BSONObjBuilder &outBuilder ) ;
   static INT32 _mthCastBasic( const CHAR *name, const BSONElement &in,
                               BSONType targetType, BSONObjBuilder &outBuilder ) ;

   static INT32 _mthSubStrByByte( const CHAR *name, const BSONElement &in,
                                  INT32 begin, INT32 limit, BOOLEAN checkLimit,
                                  BSONObjBuilder &outBuilder ) ;
   static INT32 _mthSubStrByCP( const CHAR *name, const BSONElement &in,
                                INT32 begin, INT32 limit, BOOLEAN checkLimit,
                                BSONObjBuilder &outBuilder ) ;

   static INT32 _mthSubStrByByteBasic( const CHAR *name, const BSONElement &in,
                                       INT32 begin, INT32 limit, BOOLEAN checkLimit,
                                       BSONObjBuilder &outBuilder ) ;
   static INT32 _mthSubStrByCPBasic( const CHAR *name, const BSONElement &in,
                                     INT32 begin, INT32 limit, BOOLEAN checkLimit,
                                     BSONObjBuilder &outBuilder ) ;
   static INT32 _mthConcatBasic( const CHAR *name, const BSONElement &in,
                                 const CHAR *prefix, const CHAR *suffix,
                                 BOOLEAN isReturnNull, BSONObjBuilder &outBuilder ) ;

   static INT32 _mthDayBasic( const CHAR *name, const BSONElement &in,
                              BSONObjBuilder &outBuilder ) ;

   static INT32 _mthMonthBasic( const CHAR *name, const BSONElement &in,
                                BSONObjBuilder &outBuilder ) ;

   static INT32 _mthYearBasic( const CHAR *name, const BSONElement &in,
                               BSONObjBuilder &outBuilder ) ;

   static INT32 _mthStrLenBasic( const CHAR *name, const BSONElement &in,
                                 BSONObjBuilder &outBuilder ) ;

   static BOOLEAN _mthIsUTF8StartByte( CHAR charByte ) ;
   static INT32   _mthLengthInUTF8CodePoints( const CHAR* str ) ;
   static INT32   _mthStrlenCP( const CHAR *name, const BSONElement &in,
                                BSONObjBuilder &outBuilder ) ;

   static INT32 _mthLowerBasic( const CHAR *name, const BSONElement &in,
                                BSONObjBuilder &outBuilder ) ;
   static INT32 _mthUpperBasic( const CHAR *name, const BSONElement &in,
                                BSONObjBuilder &outBuilder ) ;
   static INT32 _mthTrimBasic( const CHAR *name, const BSONElement &in, INT8 lr,
                               BSONObjBuilder &outBuilder ) ;
   static INT32 _mthAddBasic( const CHAR *name, const BSONElement &in,
                              const BSONElement &addend,
                              BSONObjBuilder &outBuilder, INT32 &flag ) ;
   static INT32 _mthSubBasic( const CHAR *name, const BSONElement &in,
                              const BSONElement &subtrahead,
                              BSONObjBuilder &outBuilder, INT32 &flag ) ;
   static INT32 _mthMultiplyBasic( const CHAR *name, const BSONElement &in,
                                   const BSONElement &multiplier,
                                   BSONObjBuilder &outBuilder, INT32 &flag ) ;
   static INT32 _mthDivideBasic( const CHAR *name, const BSONElement &in,
                                 const BSONElement &divisor,
                                 BSONObjBuilder &outBuilder, INT32 &flag ) ;

   static INT32 _mthCast( const CHAR *fieldName, const bson::BSONElement &e,
                          BSONType type, BSONObjBuilder &builder ) ;

   static void _mthGetSubStrByByte( const CHAR *src, INT32 srcLen, INT32 begin,
                                    INT32 limit, BOOLEAN checkLimit, const CHAR *&subStr,
                                    INT32 &subStrLen ) ;

   static void _mthGetSubStrByCP( const CHAR *src, INT32 srcLen, INT32 begin,
                                  INT32 limit, BOOLEAN checkLimit, const CHAR *&subStr,
                                  INT32 &subStrLen ) ;

   static INT32 _mthGetTime( const BSONElement &e, time_t &timeValue, BOOLEAN &isReturnNull ) ;

   static INT32 _lower( const CHAR *str, UINT32 len, _utilString<> &us ) ;
   static INT32 _upper( const CHAR *str, UINT32 len, _utilString<> &us ) ;

   /// lr: -1(ltrim) 0(trim) 1(rtrim)
   static void _ltrim( const CHAR *str, const CHAR *&trimed ) ;
   static INT32 _rtrim( const CHAR *str, INT32 size, _utilString<> &us ) ;
   static INT32 _mthTrim( const CHAR *str, INT32 size, INT8 lr,
                          _utilString<> &us ) ;

   static INT64 _mthRoundInt( INT64 value, INT32 scale ) ;

   static FLOAT64 _mthRoundFloat( FLOAT64 value, INT32 scale, BOOLEAN &isSpecial ) ;

   static INT32 _mthFormatString( const CHAR *src, INT32 scale, ossPoolString &result,
                                  BOOLEAN isFromInt ) ;

   const static INT64 _mthLog10INT64[] = {
      0x0000000000000001, 0x000000000000000A, 0x0000000000000064, 0x00000000000003E8,
      0x0000000000002710, 0x00000000000186A0, 0x00000000000F4240, 0x0000000000989680,
      0x0000000005F5E100, 0x000000003B9ACA00, 0x00000002540BE400, 0x000000174876E800,
      0x000000E8D4A51000, 0x000009184E72A000, 0x00005AF3107A4000, 0x00038D7EA4C68000,
      0x002386F26FC10000, 0x016345785D8A0000, 0x0DE0B6B3A7640000
      } ;

   const static UINT32 _mthLog10INT64Size =
      (UINT32) ( sizeof( _mthLog10INT64 ) / sizeof( _mthLog10INT64[0] ) ) ;

   const static FLOAT64 _mthLog10FLOAT64[] = {
      1e000, 1e001, 1e002, 1e003, 1e004, 1e005, 1e006, 1e007, 1e008, 1e009,
      1e010, 1e011, 1e012, 1e013, 1e014, 1e015, 1e016, 1e017, 1e018, 1e019,
      1e020, 1e021, 1e022, 1e023, 1e024, 1e025, 1e026, 1e027, 1e028, 1e029,
      1e030, 1e031, 1e032, 1e033, 1e034, 1e035, 1e036, 1e037, 1e038, 1e039,
      1e040, 1e041, 1e042, 1e043, 1e044, 1e045, 1e046, 1e047, 1e048, 1e049,
      1e050, 1e051, 1e052, 1e053, 1e054, 1e055, 1e056, 1e057, 1e058, 1e059,
      1e060, 1e061, 1e062, 1e063, 1e064, 1e065, 1e066, 1e067, 1e068, 1e069,
      1e070, 1e071, 1e072, 1e073, 1e074, 1e075, 1e076, 1e077, 1e078, 1e079,
      1e080, 1e081, 1e082, 1e083, 1e084, 1e085, 1e086, 1e087, 1e088, 1e089,
      1e090, 1e091, 1e092, 1e093, 1e094, 1e095, 1e096, 1e097, 1e098, 1e099,
      1e100, 1e101, 1e102, 1e103, 1e104, 1e105, 1e106, 1e107, 1e108, 1e109,
      1e110, 1e111, 1e112, 1e113, 1e114, 1e115, 1e116, 1e117, 1e118, 1e119,
      1e120, 1e121, 1e122, 1e123, 1e124, 1e125, 1e126, 1e127, 1e128, 1e129,
      1e130, 1e131, 1e132, 1e133, 1e134, 1e135, 1e136, 1e137, 1e138, 1e139,
      1e140, 1e141, 1e142, 1e143, 1e144, 1e145, 1e146, 1e147, 1e148, 1e149,
      1e150, 1e151, 1e152, 1e153, 1e154, 1e155, 1e156, 1e157, 1e158, 1e159,
      1e160, 1e161, 1e162, 1e163, 1e164, 1e165, 1e166, 1e167, 1e168, 1e169,
      1e170, 1e171, 1e172, 1e173, 1e174, 1e175, 1e176, 1e177, 1e178, 1e179,
      1e180, 1e181, 1e182, 1e183, 1e184, 1e185, 1e186, 1e187, 1e188, 1e189,
      1e190, 1e191, 1e192, 1e193, 1e194, 1e195, 1e196, 1e197, 1e198, 1e199,
      1e200, 1e201, 1e202, 1e203, 1e204, 1e205, 1e206, 1e207, 1e208, 1e209,
      1e210, 1e211, 1e212, 1e213, 1e214, 1e215, 1e216, 1e217, 1e218, 1e219,
      1e220, 1e221, 1e222, 1e223, 1e224, 1e225, 1e226, 1e227, 1e228, 1e229,
      1e230, 1e231, 1e232, 1e233, 1e234, 1e235, 1e236, 1e237, 1e238, 1e239,
      1e240, 1e241, 1e242, 1e243, 1e244, 1e245, 1e246, 1e247, 1e248, 1e249,
      1e250, 1e251, 1e252, 1e253, 1e254, 1e255, 1e256, 1e257, 1e258, 1e259,
      1e260, 1e261, 1e262, 1e263, 1e264, 1e265, 1e266, 1e267, 1e268, 1e269,
      1e270, 1e271, 1e272, 1e273, 1e274, 1e275, 1e276, 1e277, 1e278, 1e279,
      1e280, 1e281, 1e282, 1e283, 1e284, 1e285, 1e286, 1e287, 1e288, 1e289,
      1e290, 1e291, 1e292, 1e293, 1e294, 1e295, 1e296, 1e297, 1e298, 1e299,
      1e300, 1e301, 1e302, 1e303, 1e304, 1e305, 1e306, 1e307, 1e308
   } ;

   const static UINT32 _mthLog10FLOAT64Size =
      (UINT32) ( sizeof( _mthLog10FLOAT64 ) / sizeof( _mthLog10FLOAT64[0] ) ) ;

   INT32 _mthCast( const CHAR *fieldName, const bson::BSONElement &e,
                   BSONType type, BSONObjBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( e.type() != type, "should not be same" ) ;
      switch ( type )
      {
      case MinKey :
         builder.appendMinKey( fieldName ) ;
         break ;
      case EOO :
         rc = SDB_INVALIDARG ;
         break ;
      case NumberDouble :
      {
         if ( Date == e.type() )
         {
            builder.appendNumber( fieldName,
                                  (FLOAT64)( ( INT64 )( e.date().millis ) ) ) ;
         }
         else if ( Timestamp == e.type() )
         {
            INT64 l = ( INT64 )( e.timestampTime().millis ) ;
            l += ( INT64 )( e.timestampInc() / 1000 ) ;
            builder.appendNumber( fieldName, (FLOAT64)l ) ;
         }
         else if ( Bool == e.type() )
         {
            FLOAT64 f = e.Bool() ? 1.0 : 0.0 ;
            builder.appendNumber( fieldName, f ) ;
         }
         else if ( String != e.type() )
         {
            FLOAT64 f = e.numberDouble() ;
            if ( isInf( f ) )
            {
               f = 0.0 ;
            }
            builder.appendNumber( fieldName, f ) ;
         }
         else
         {
            try
            {
               FLOAT64 f = 0.0 ;
               f = boost::lexical_cast<FLOAT64>( e.valuestr () ) ;
               builder.appendNumber( fieldName, f ) ;
            }
            catch ( boost::bad_lexical_cast & )
            {
               builder.appendNumber( fieldName, 0.0 ) ;
            }
         }
         break ;
      }
      case String :
      {
         if ( NumberInt == e.type() )
         {
            _utilString<UTIL_STRING_INT_LEN+1> us ;
            rc = us.appendINT32( e.numberInt() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append int32:%d", rc ) ;
               goto error ;
            }
            builder.append( fieldName, us.str() ) ;
         }
         else if ( NumberLong == e.type() )
         {
            _utilString<UTIL_STRING_INT64_LEN+1> us ;
            rc = us.appendINT64( e.numberLong() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append int64:%d", rc ) ;
               goto error ;
            }
            builder.append( fieldName, us.str() ) ;
         }
         else if ( NumberDouble == e.type() )
         {
            _utilString<UTIL_STRING_DOUBLE_LEN+1> us ;
            rc = us.appendDouble( e.numberDouble() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append float64:%d", rc ) ;
               goto error ;
            }
            builder.append( fieldName, us.str() ) ;
         }
         else if ( NumberDecimal == e.type() )
         {
            _utilString<> us ;
            bsonDecimal decimal ;
            string value ;

            decimal = e.numberDecimal() ;
            rc = decimal.toStringChecked( value ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed toStringChecked,rc=%d",rc ) ;
               goto error ;
            }

            rc      = us.append( value.c_str(), value.length() );
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append decimal=%s,rc=%d",
                       value.c_str(), rc ) ;
               goto error ;
            }
            builder.append( fieldName, us.str() ) ;
         }
         else if ( Date == e.type() )
         {
            CHAR buffer[64] = { 0 };
            time_t timer = (time_t)( ( INT64 )( e.date() ) / 1000 ) ;
            struct tm psr ;
            local_time ( &timer, &psr ) ;
            sprintf ( buffer,
                      "%04d-%02d-%02d",
                      psr.tm_year + 1900,
                      psr.tm_mon + 1,
                      psr.tm_mday ) ;
            builder.append( fieldName, buffer ) ;
         }
         else if ( Timestamp == e.type() )
         {
            Date_t date = e.timestampTime () ;
            unsigned int inc = e.timestampInc () ;
            char buffer[128] = { 0 };
            time_t timer = (time_t)((( INT64 )(date.millis))/1000) ;
            struct tm psr ;
            local_time ( &timer, &psr ) ;
            sprintf ( buffer,
                      "%04d-%02d-%02d-%02d.%02d.%02d.%06d",
                      psr.tm_year + 1900,
                      psr.tm_mon + 1,
                      psr.tm_mday,
                      psr.tm_hour,
                      psr.tm_min,
                      psr.tm_sec,
                      inc ) ;
            builder.append( fieldName, buffer ) ;
         }
         else if ( jstOID == e.type() )
         {
            builder.append( fieldName, e.OID().str() ) ;
         }
         else if ( Object == e.type() )
         {
            builder.append( fieldName,
                            e.embeddedObject().toString( FALSE, TRUE ) ) ;
         }
         else if ( Array == e.type() )
         {
            builder.append( fieldName,
                            e.embeddedObject().toString( TRUE, TRUE ) ) ;
         }
         else if ( Bool == e.type() )
         {
            builder.append( fieldName,
                            e.booleanSafe() ?
                            "true" : "false" ) ;
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }
      case Object :
      {
         if ( String == e.type() )
         {
            BSONObj obj ;
            INT32 r = fromjson( e.valuestr(), obj ) ;
            if ( SDB_OK == r )
            {
               builder.append( fieldName, obj ) ;
            }
            else
            {
               builder.appendNull( fieldName ) ;
            }
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }
      case Array :
      case BinData :
      case Undefined :
         builder.appendNull( fieldName ) ;
         break ;
      case jstOID :
      {
         if ( String == e.type() &&
              25 == e.valuestrsize() )
         {
            bson::OID o( e.valuestr() ) ;
            builder.appendOID( fieldName, &o ) ;
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }
      case Bool :
         builder.appendBool( fieldName, e.trueValue() ) ;
         break ;
      case Date :
      {
         UINT64 tm = 0 ;
         if ( e.isNumber() )
         {
            if ( NumberInt == e.type() )
            {
               Date_t d( e.numberInt() * 1000LL ) ;
               builder.appendDate( fieldName, d ) ;
            }
            else
            {
               BOOLEAN hasAppend = FALSE ;
               if ( NumberDecimal == e.type() )
               {
                  bsonDecimal original = e.Decimal() ;
                  bsonDecimal l_min ;
                  bsonDecimal l_max ;
                  rc = l_min.fromLong( OSS_SINT64_MIN ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to parse decimal:rc=%d", rc ) ;
                     goto error ;
                  }

                  rc = l_max.fromLong( OSS_SINT64_MAX ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to parse decimal:rc=%d", rc ) ;
                     goto error ;
                  }

                  if ( original.compare( l_min ) < 0 ||
                       original.compare( l_max ) > 0 )
                  {
                     builder.appendNull( fieldName ) ;
                     hasAppend = TRUE ;
                  }
               }
               if ( FALSE == hasAppend )
               {
                  Date_t d( e.numberLong() ) ;
                  builder.appendDate( fieldName, d ) ;
               }
            }
         }
         else if ( String == e.type() &&
                   SDB_OK == utilStr2Date( e.valuestr(), tm ))
         {
            builder.appendDate( fieldName, Date_t( tm ) ) ;
         }
         else if ( Timestamp == e.type() )
         {
            builder.appendDate( fieldName, e.timestampTime() ) ;
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }
      case jstNULL :
      case RegEx :
      case DBRef :
      case Code :
      case Symbol :
      case CodeWScope :
         builder.appendNull( fieldName ) ;
         break ;
      case NumberInt :
      {
         if ( Date == e.type() )
         {
            INT32 sec = 0 ;
            INT64 l   = ( ( INT64 )( e.date().millis ) ) / 1000 ;
            if ( l > OSS_SINT32_MAX_LL || l < OSS_SINT32_MIN_LL )
            {
               sec = 0 ;
            }
            else
            {
               sec = ( INT32 )l ;
            }
            builder.appendNumber( fieldName, sec ) ;
         }
         else if ( Timestamp == e.type() )
         {
            INT32 sec = 0 ;
            INT64 l = ( INT64 )( e.timestampTime().millis ) ;
            l      += ( INT64 )( ((INT32)(e.timestampInc())) / 1000 ) ;
            l       = l / 1000; // seconds
            if ( l > OSS_SINT32_MAX_LL || l < OSS_SINT32_MIN_LL )
            {
               sec = 0 ;
            }
            else
            {
               sec = (INT32)l ;
            }
            builder.appendNumber( fieldName, sec ) ;
         }
         else if ( Bool == e.type() )
         {
            INT32 v = e.Bool() ? 1 : 0 ;
            builder.append( fieldName, v ) ;
         }
         else if ( NumberLong == e.type() )
         {
            INT32 i = 0 ;
            INT64 l = e.numberLong() ;
            if ( l > OSS_SINT32_MAX_LL || l < OSS_SINT32_MIN_LL )
            {
               i = 0 ;
            }
            else
            {
               i = ( INT32 )l ;
            }
            builder.appendNumber( fieldName, i ) ;
         }
         else if ( NumberDecimal == e.type() )
         {
            INT32 i = 0 ;
            INT64 l = 0 ;
            rc = e.numberDecimal().toLong( &l) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:rc=%d", rc ) ;
               goto error ;
            }
            if ( l > OSS_SINT32_MAX_LL || l < OSS_SINT32_MIN_LL )
            {
               i = 0 ;
            }
            else
            {
               i = ( INT32 )l ;
            }
            builder.appendNumber( fieldName, i ) ;
         }
         else if ( NumberDouble == e.type() )
         {
            INT32 i = 0 ;
            double d = e.Double() ;
            if ( d > OSS_SINT32_MAX_D || d < OSS_SINT32_MIN_D )
            {
               i = 0 ;
            }
            else
            {
               i = ( INT32 )d ;
            }
            builder.appendNumber( fieldName, i ) ;
         }
         else if ( String != e.type() )
         {
            builder.appendNumber( fieldName, e.numberInt() ) ;
         }
         else
         {
            try
            {
               INT32 i = 0 ;
               double v = 0 ;
               v = boost::lexical_cast<double>( e.valuestr () ) ;
               if ( v > OSS_SINT32_MAX_D || v < OSS_SINT32_MIN_D )
               {
                  i = 0 ;
               }
               else
               {
                  i = ( INT32 )v ;
               }
               builder.appendNumber( fieldName, i ) ;
            }
            catch ( boost::bad_lexical_cast & )
            {
               builder.appendNumber( fieldName, 0 ) ;
            }
         }
         break ;
      }
      case Timestamp :
      {
         time_t tm = 0 ;;
         UINT64 usec = 0 ;
         if ( e.isNumber() )
         {
            if ( NumberInt == e.type() )
            {
               INT32 sec = ( INT32 )( e.numberInt() ) ; // take it as seconds
               OpTime t( (unsigned) (sec), 0 ) ;
               builder.appendTimestamp( fieldName, t.asDate() ) ;
            }
            else
            {
               BOOLEAN hasAppend = FALSE ;
               if ( NumberDecimal == e.type() )
               {
                  bsonDecimal original = e.Decimal() ;
                  bsonDecimal l_min ;
                  bsonDecimal l_max ;
                  rc = l_min.fromLong( OSS_SINT64_MIN ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to parse decimal:rc=%d", rc ) ;
                     goto error ;
                  }

                  rc = l_max.fromLong( OSS_SINT64_MAX ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDERROR, "Failed to parse decimal:rc=%d", rc ) ;
                     goto error ;
                  }

                  if ( original.compare( l_min ) < 0 ||
                       original.compare( l_max ) > 0 )
                  {
                     builder.appendNull( fieldName ) ;
                     hasAppend = TRUE ;
                  }
               }
               if ( FALSE == hasAppend )
               {
                  INT64 varLong = ( INT64 )( e.numberLong() ) ;
                  INT64 sec     = varLong / 1000 ;
                  INT64 us      = ( varLong % 1000 ) * 1000 ; // microseconds
                  if ( us < 0 )
                  {
                     // move 1s from sec to us
                     sec--;
                     us += 1000000;
                  }
                  if ( sec > OSS_SINT32_MAX_LL || sec < OSS_SINT32_MIN_LL )
                  {
                     builder.appendNull( fieldName ) ;
                  }
                  else
                  {
                     OpTime t( (unsigned) (sec), (unsigned) (us) ) ;
                     builder.appendTimestamp( fieldName, t.asDate() ) ;
                  }
               }
            }
         }
         else if ( String == e.type() &&
                   SDB_OK == engine::utilStr2TimeT( e.valuestr(),
                                                    tm,
                                                    &usec ))
         {
            OpTime t( (unsigned) (tm) , usec );
            builder.appendTimestamp( fieldName, t.asDate() ) ;
         }
         else if ( Date == e.type() )
         {
            // when date is large than the max value of timestamp,
            // return null
            INT64 sec = ( ( INT64 )( e.date().millis ) ) / 1000 ;
            if ( sec > OSS_SINT32_MAX_LL || sec < OSS_SINT32_MIN_LL )
            {
               builder.appendNull( fieldName ) ;
            }
            else
            {
               OpTime t( (unsigned) (sec), 0 ) ;
               builder.appendTimestamp( fieldName, t.asDate() ) ;
            }
         }
         else
         {
            builder.appendNull( fieldName ) ;
         }
         break ;
      }
      case NumberLong :
      {
         if ( Date == e.type() )
         {
            builder.appendNumber( fieldName,
                                  ( INT64 )( e.date().millis ) ) ;
         }
         else if ( Timestamp == e.type() )
         {
            INT64 l = ( INT64 )( e.timestampTime().millis ) ;
            l += ( INT64 )( e.timestampInc() / 1000 ) ;
            builder.appendNumber( fieldName, l ) ;
         }
         else if ( Bool == e.type() )
         {
            INT64 v = e.Bool() ? 1 : 0 ;
            builder.append( fieldName, v ) ;
         }
         else if ( NumberDouble == e.type() )
         {
            INT64 l = 0 ;
            double d = e.Double() ;
            if ( d >= 0 && d < (OSS_SINT64_MAX_D + 1) )
            {
               l = (INT64)d ;
            }
            else if ( d < 0 && d >= OSS_SINT64_MIN_D )
            {
               l = (INT64)d ;
            }
            builder.appendNumber( fieldName, l ) ;
         }
         else if ( String != e.type() )
         {
            builder.appendNumber( fieldName, e.numberLong() ) ;
         }
         else
         {
            try
            {
               //if the STRING has "." "e" or "E" use double type
               if ( ossStrchr ( e.valuestr (), '.' ) != NULL ||
                    ossStrchr ( e.valuestr (), 'E' ) != NULL ||
                    ossStrchr ( e.valuestr (), 'e' ) != NULL )
               {
                  double d = 0  ;
                  INT64 l = 0 ;
                  d = boost::lexical_cast<double>( e.valuestr () ) ;
                  if ( d >= 0 && d < (OSS_SINT64_MAX_D + 1) )
                  {
                     l = (INT64)d ;
                  }
                  else if ( d < 0 && d >= OSS_SINT64_MIN_D )
                  {
                     l = (INT64)d ;
                  }
                  builder.appendNumber( fieldName, l ) ;
               }
               else
               {
                  INT64 l = 0 ;
                  l = boost::lexical_cast<INT64>( e.valuestr () ) ;
                  builder.appendNumber( fieldName, l ) ;
               }
            }
            catch ( boost::bad_lexical_cast & )
            {
               builder.appendNumber( fieldName, 0 ) ;
            }
         }
         break ;
      }
      case NumberDecimal :
      {
         if ( Date == e.type() )
         {
            bsonDecimal decimal ;
            rc = decimal.fromLong( ( INT64 )( e.date().millis ) ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:e=%s,rc=%d",
                       e.String().c_str(), rc ) ;
               goto error ;
            }
            builder.append( fieldName, decimal ) ;
         }
         else if ( Timestamp == e.type() )
         {
            bsonDecimal decimal ;
            INT64 l = ( INT64 )( e.timestampTime().millis ) ;
            l      += ( INT64 )( ( (INT32)(e.timestampInc()) ) / 1000 ) ;
            rc = decimal.fromLong( l ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:e=%s,rc=%d",
                       e.String().c_str(), rc ) ;
               goto error ;
            }
            builder.append( fieldName, decimal ) ;
         }
         else if ( Bool == e.type() )
         {
            bsonDecimal decimal ;
            INT64 v = e.Bool() ? 1 : 0 ;

            rc = decimal.fromLong( ( INT64 )v ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:e=%s,rc=%d",
                       e.String().c_str(), rc ) ;
               goto error ;
            }
            builder.append( fieldName, decimal ) ;
         }
         else if ( NumberLong == e.type() )
         {
            bsonDecimal decimal ;
            rc = decimal.fromLong( e.numberLong() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:e=%s,rc=%d",
                       e.String().c_str(), rc ) ;
               goto error ;
            }
            builder.append( fieldName, decimal ) ;
         }
         else if ( String != e.type() )
         {
            bsonDecimal decimal ;
            rc = decimal.fromDouble( e.numberDouble() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:e=%s,rc=%d",
                       e.String().c_str(), rc ) ;
               goto error ;
            }
            builder.append( fieldName, decimal ) ;
         }
         else
         {
            bsonDecimal decimal ;
            rc = decimal.fromString( e.String().c_str() ) ;
            if ( SDB_OK != rc && SDB_INVALIDARG != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:e=%s,rc=%d",
                       e.String().c_str(), rc ) ;
               goto error ;
            }

            rc = SDB_OK ;
            // SDB_OK or SDB_INVALIDARG(invalid string return the default value)
            builder.append( fieldName, decimal ) ;
         }
         break ;
      }
      case MaxKey :
         builder.appendMaxKey( fieldName ) ;
         break ;
      default:
         rc = SDB_INVALIDARG ;
         break ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "invalid cast type:%d", type ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _mthGetSubStrByByte( const CHAR *src, INT32 srcLen, INT32 begin, INT32 limit,
                             BOOLEAN checkLimit, const CHAR *&subStr, INT32 &subStrLen )
   {
      const CHAR *cpBegin = NULL ;

      // $leftBytes or $rightBytes args is negative number
      if ( srcLen < 0 ||
           ( checkLimit && ( begin > 0 || limit < 0 ) ) )
      {
         goto error ;
      }

      if ( 0 <= begin )
      {
         if ( srcLen <= begin )
         {
            goto error ;
         }
         cpBegin = src + begin ;
         subStrLen = srcLen - begin ;
      }
      else
      {
         INT32 beginPos = srcLen + begin ;
         // $rightBytes len args over left boundary
         if ( beginPos < 0 && checkLimit )
         {
            beginPos = 0 ;
         }
         else if ( beginPos < 0 )
         {
            goto error ;
         }
         cpBegin = src + beginPos ;
         subStrLen = srcLen - beginPos ;
      }

      if ( 0 <= limit && limit < subStrLen )
      {
         subStrLen = limit ;
      }

      subStr = cpBegin ;
   done:
      return ;
   error:
      subStr    = NULL ;
      subStrLen = -1 ;
      goto done ;
   }

   void _mthGetSubStrByCP( const CHAR *src, INT32 srcLen, INT32 begin, INT32 limit,
                           BOOLEAN checkLimit, const CHAR *&subStr, INT32 &subStrLen )
   {
      INT32 bytesStart = 0, bytesEnd = 0 ;
      INT32 cpBegin = 0, cpLen = 0 ;
      INT32 i = 0 ;
      const CHAR *subStrBegin = NULL ;

      // $leftCP or $rightCP args is negative number
      if ( srcLen < 0 ||
           ( checkLimit && ( begin > 0 || limit < 0 ) ) )
      {
         goto error ;
      }

      // interception substring step:
      // the first step finds where the substring byte starts
      // the second step finds where the substring byte ends
      if ( 0 <= begin )
      {
         if ( srcLen <= begin )
         {
            goto error ;
         }
         for ( i = 0; i <= srcLen && cpBegin <= begin; i++ )
         {
            if ( _mthIsUTF8StartByte( src[i] ) )
            {
               ++ cpBegin ;
            }
         }
         bytesStart = i - 1 ;
         subStrBegin = src + bytesStart ;

         // 1. $substrCP args <len> is negative number
         // 2. substring more than srcStr len
         if ( limit < 0 || limit > srcLen )
         {
            bytesEnd = srcLen ;
            subStrLen = bytesEnd - bytesStart ;
         }
         else
         {
            for ( i = bytesStart; i <= srcLen && cpLen <= limit; i++ )
            {
               if ( _mthIsUTF8StartByte( src[i] ) )
               {
                  ++ cpLen ;
               }
            }

            bytesEnd = i - 1 ;
            subStrLen = bytesEnd - bytesStart ;
         }
      }
      else
      {
         for ( i = srcLen - 1; i >= 0 && cpBegin > begin; i-- )
         {
            if ( _mthIsUTF8StartByte( src[i] ) )
            {
               -- cpBegin ;
            }
         }
         // $substrCP pos args over left boundary
         if ( !checkLimit && cpBegin > begin )
         {
            goto error ;
         }
         bytesStart = i + 1 ;
         subStrBegin = src + bytesStart ;

         // 1. $substrCP args <len> is negative number
         // 2. substring more than srcStr len
         if ( limit < 0 || limit > srcLen )
         {
            bytesEnd = srcLen ;
            subStrLen = bytesEnd - bytesStart ;
         }
         else
         {
            for ( i = bytesStart; i <= srcLen && cpLen <= limit; i++ )
            {
               if ( _mthIsUTF8StartByte( src[i] ) )
               {
                  ++ cpLen ;
               }
            }
            bytesEnd = i - 1 ;
            subStrLen = bytesEnd - bytesStart ;
         }
      }

      subStr = subStrBegin ;
   done:
      return ;
   error:
      subStr = NULL ;
      subStrLen = -1 ;
      goto done ;
   }


   INT32 _lower( const CHAR *str, UINT32 len, _utilString<> &us )
   {
      INT32 rc = SDB_OK ;
      us.resize( len ) ;
      for ( UINT32 i = 0; i < len; ++i )
      {
         const CHAR *p = str + i ;
         if ( 'A' <= *p &&
              *p <= 'Z' )
         {
            rc = us.append( *p + 32 ) ;
         }
         else
         {
             rc = us.append( *p ) ;
         }

         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append str:%d", rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _upper( const CHAR *str, UINT32 len, _utilString<> &us )
   {
      INT32 rc = SDB_OK ;
      us.resize( len ) ;
      for ( UINT32 i = 0; i < len; ++i )
      {
         const CHAR *p = str + i ;
         if ( 'a' <= *p &&
              *p <= 'z' )
         {
            rc = us.append( *p - 32 ) ;
         }
         else
         {
             rc = us.append( *p ) ;
         }

         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append str:%d", rc ) ;
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   void _ltrim( const CHAR *str, const CHAR *&trimed )
   {
      const CHAR *p = str ;
      while ( '\0' != *p )
      {
         if ( ' ' != *p && '\t' != *p && '\n' != *p && '\r' != *p )
         {
            break ;
         }

         ++p ;
      }

      trimed = p ;
      return ;
   }

   INT32 _rtrim( const CHAR *str, INT32 size, _utilString<> &us )
   {
      INT32 rc  = SDB_OK ;
      INT32 pos = size - 1 ;

      while ( 0 <= pos )
      {
         const CHAR *p = str + pos ;
         if ( ' ' != *p && '\t' != *p && '\n' != *p && '\r' != *p )
         {
            break ;
         }

         --pos ;
      }

      if ( 0 <= pos )
      {
         rc = us.append( str, pos + 1 ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append string:%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT64 _mthRoundInt( INT64 value, INT32 scale )
   {
      BOOLEAN scaleNegative = scale < 0 ;
      volatile INT64 result = 0 ;
      /// if scale is negative, divide by the scale pow of 10,
      /// and get the remainder to determine whether to carry
      if ( scaleNegative )
      {
         UINT32 absScale = -scale ;
         if ( absScale < _mthLog10INT64Size )
         {
            INT64 tmp = _mthLog10INT64[absScale] ;
            INT64 carry = 0 ;
            /// pre-compute these, to avoid optimizing away '(v/tmp) * tmp'.
            volatile INT64 valueDivTmp = value / tmp ;
            /// get the remainder to determine whether to carry
            volatile INT64 valueModTmp = value % tmp ;
            if ( 0 > value && tmp >> 1 <= -valueModTmp )
            {
               carry = -tmp ;
            }
            else if ( 0 < value && tmp >> 1 <= valueModTmp )
            {
               carry = tmp ;
            }
            result = valueDivTmp * tmp + carry ;
         }
         else
         {
            result = 0 ;
         }
      }
      /// if scale is positive, no need to calculate, so return original value
      else
      {
         result = value ;
      }
      return result ;
   }

   FLOAT64 _mthRoundFloat( FLOAT64 value, INT32 scale, BOOLEAN &isSpecial )
   {
      volatile FLOAT64 result = 0.0 ;
      /// if value is Inf or NaN, we should return origin value.
      if ( ossIsInf( value ) || ossIsNaN( value ) )
      {
         result = value ;
         isSpecial = TRUE ;
      }
      else
      {
         FLOAT64 tmp = 0.0 ;
         BOOLEAN scaleNegative = scale < 0 ;
         /// positive scale
         UINT32 absScale = scaleNegative ? -scale : scale ;
         /// the scale pow of 10
         tmp = absScale < _mthLog10FLOAT64Size ? _mthLog10FLOAT64[absScale] :
                                                 pow( 10.0, (FLOAT64)absScale ) ;
         /// pre-compute these, to avoid optimizing away e.g. 'round(v/tmp) * tmp'.
         volatile FLOAT64 valueDivTmp = value / tmp ;
         volatile FLOAT64 valueMulTmp = value * tmp ;
         /* if scale is negative and tmp is Inf or NaN, it means that the
          * precision of FLOAT64 is overflowed after rounding.
          * we should return 0.0f.
          */
         if ( scaleNegative && ossIsInf( tmp ) )
         {
            result = 0.0 ;
         }
         /* if scale is positive and pre-compute is Inf or NaN, it means that the
          * precision of FLOAT64 is overflowed before rounding.
          * we should return original value.
          */
         else if ( !scaleNegative &&
                   ( ossIsInf( valueMulTmp ) || ossIsNaN( valueMulTmp ) ) )
         {
            result = value ;
         }
         else
         {
            result = scaleNegative ? OSS_ROUND( valueDivTmp ) * tmp :
                                     OSS_ROUND( valueMulTmp ) / tmp ;
         }
         isSpecial = FALSE ;
      }
      return result ;
   }

   INT32 _mthFormatString( const CHAR * src, INT32 scale, ossPoolString &result,
                           BOOLEAN isFromInt )
   {
      INT32 rc = SDB_OK ;
      INT32 sign = *src == '-' ? 1 : 0 ;
      const utilNumberFacet& myNumberFacet = utilGetDefaultNumberFacet() ;
      CHAR decimalPoint = myNumberFacet.decimalPoint() ;
      CHAR thousandsSep = myNumberFacet.thousandsSep() ;
      const CHAR *grouping = myNumberFacet.grouping() ;
      INT32 orgLen = ossStrlen( src ) ;

      try
      {
         ossPoolString tmp ;
         INT32 curPos = 0 ;
         INT32 extendLen = 0 ;
         const CHAR *n = src + orgLen - 1 ;
         CHAR *tmpData = NULL ;
         /// replace the decimal point
         if ( 0 < scale )
         {
            if ( MTH_FORMAT_MAX_SCALE < scale )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "scale(%d) must be less than %d, "
                       "rc:%d", scale, MTH_FORMAT_MAX_SCALE, rc ) ;
               goto error ;
            }
            if ( isFromInt )
            {
               extendLen = 2 * orgLen + scale + 2 ;
               curPos = extendLen - 1 ;
               tmp.reserve( extendLen ) ;
               tmpData = ( CHAR* )tmp.data() ;
               tmpData[ curPos ] = '\0' ;
               curPos -= ( scale + 1 ) ;
               tmpData[ curPos ] = decimalPoint ;
               for ( INT32 i = 1 ; i <= scale ; ++i )
               {
                  tmpData[ curPos + i ] = '0' ;
               }
            }
            else
            {
               const CHAR *p = NULL ;
               INT32 i = 1 ;
               INT32 pointPos = scale + 1 ;
               extendLen = 2 * orgLen + 1 ;
               curPos = extendLen - 1 ;
               tmp.reserve( extendLen ) ;
               tmpData = ( CHAR* )tmp.data() ;
               tmpData[ curPos ] = '\0' ;
               curPos -= pointPos ;
               SDB_ASSERT( '.' == src[ orgLen - pointPos ], "must be \'.\'" ) ;
               tmpData[ curPos ] = decimalPoint ;
               for ( p = src + orgLen - pointPos + i ;
                     i <= scale && p < src + orgLen ; ++i, ++p )
               {
                  tmpData[ curPos + i ] = *p ;
               }
               SDB_ASSERT( p == src + orgLen, "must point the end of src" ) ;
               n -= pointPos ;
            }
         }
         else
         {
            extendLen = 2 * orgLen + 1 ;
            curPos = extendLen - 1 ;
            tmp.reserve( extendLen ) ;
            tmpData = ( CHAR* )tmp.data() ;
            tmpData[ curPos ] = '\0' ;
         }

         if ( 0 < grouping[0] && '\0' != thousandsSep )
         {
            /// replace the integer part with grouping
            for ( INT32 count = *grouping ; n >= ( src + sign ) ; --count )
            {
               /// *grouping==0x03  means "digit grouping in numeric is 3".
               if ( 0 == count )
               {
                  tmpData[ --curPos ] = thousandsSep ;

                  if ( grouping[1] )
                  {
                     grouping++ ;
                  }
                  count = *grouping ;
               }
               tmpData[ --curPos ] = *n-- ;
            }
         }
         else
         {
            /// replace the integer part without grouping
            while ( n >= ( src + sign ) )
            {
               tmpData[ --curPos ] = *n-- ;
            }
         }
         /// put '-'
         if ( sign )
         {
            tmpData[ --curPos ] = *src ;
         }
         result.assign( tmpData + curPos ) ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Failed to append format string, occurred "
                 "unexpected exception: %s, rc:%d", e.what(), rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthTrim( const CHAR *str, INT32 size, INT8 lr, _utilString<> &us )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != str, "can not be null" ) ;
      INT32 strLen = 0 <= size ? size : ossStrlen( str ) ;
      const CHAR *p = str ;
      if ( 0 == strLen )
      {
         goto done ;
      }

      if ( lr <= 0 )
      {
         const CHAR *newP = NULL ;
         _ltrim( p, newP ) ;
         p = newP ;
      }

      if ( 0 <= lr )
      {
         rc = _rtrim( p, size - ( p - str ), us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to trim right site:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         /// necessary to avoid one more copy when
         /// str is like "  abc" ?
         rc = us.append( p, size - ( p - str ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to trim right site:%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // the function try to append newStr to ppStr.
   // if the buffer is not large enough the function is responsible to allocate
   // a larger one. If failed to allocate larger buffer, this function must
   // maintain the validity of original pointer
   INT32 mthAppendString ( CHAR **ppStr, INT32 &bufLen,
                           INT32 strLen, const CHAR *newStr,
                           INT32 newStrLen, INT32 *pMergedLen )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT ( ppStr && newStr, "str or newStr can't be NULL" ) ;
      // if user doesn't know the string length, pass 0
      if ( !*ppStr )
      {
         strLen = 0 ;
      }
      else if ( strLen <= 0 )
      {
         strLen = ossStrlen ( *ppStr ) ;
      }
      // if user doesn't know the new string len, pass 0
      if ( newStrLen <= 0 )
      {
         newStrLen = ossStrlen ( newStr ) ;
      }
      // make sure the string len and new string len is less than buffer
      if ( strLen + newStrLen >= bufLen )
      {
         // we need to allocate more memory if exceed buffer
         CHAR *pOldStr = *ppStr ;
         INT32 newSize = ossRoundUpToMultipleX ( strLen + newStrLen,
                                                 SDB_PAGE_SIZE ) ;
         if ( newSize < 0 )
         {
            PD_LOG ( PDERROR, "new buffer overflow, size: %d", newSize ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         *ppStr = (CHAR*)SDB_OSS_REALLOC ( *ppStr, sizeof(CHAR)*(newSize) ) ;
         if ( !*ppStr )
         {
            PD_LOG ( PDERROR, "Failed to allocate %d bytes buffer", newSize ) ;
            rc = SDB_OOM ;
            *ppStr = pOldStr ;
            goto error ;
         }
         bufLen = newSize ;
      }
      // now new buffer is allocated or we already have enough memory, let's do
      // copy
      if ( *ppStr && newStr )
      {
         ossMemcpy ( &(*ppStr)[strLen], newStr, newStrLen ) ;
         (*ppStr)[strLen+newStrLen] = '\0' ;

         if ( pMergedLen )
         {
            *pMergedLen = strLen + newStrLen ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   BOOLEAN mthIsZero( const BSONElement &ele )
   {
      if ( ele.type() == NumberDecimal ) {
         bsonDecimal decimal = ele.numberDecimal() ;
         if ( decimal.isZero() ) {
            return TRUE ;
         }
      }
      else if ( ele.type() == NumberDouble ) {
         double d = ele.numberDouble() ;
         if ( d < OSS_EPSILON && d > -OSS_EPSILON ) {
            return TRUE ;
         }
      }
      else {
         long l = ele.numberLong() ;
         if ( 0 == l ) {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__MTHDOUBLEBUFFERSIZE, "mthDoubleBufferSize" )
   INT32 mthDoubleBufferSize ( CHAR **ppStr, INT32 &bufLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__MTHDOUBLEBUFFERSIZE ) ;
      SDB_ASSERT ( ppStr, "ppStr can't be NULL" ) ;
      CHAR *pOldStr = *ppStr ;
      INT32 newSize = ossRoundUpToMultipleX ( 2*bufLen,
                                              SDB_PAGE_SIZE ) ;
      if ( newSize < 0 )
      {
         PD_LOG ( PDERROR, "new buffer overflow" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( 0 == newSize )
      {
         newSize = SDB_PAGE_SIZE ;
      }
      *ppStr = (CHAR*)SDB_OSS_REALLOC ( *ppStr, sizeof(CHAR)*(newSize) ) ;
      if ( !*ppStr )
      {
         PD_LOG ( PDERROR, "Failed to allocate %d bytes buffer", newSize ) ;
         rc = SDB_OOM ;
         *ppStr = pOldStr ;
         goto error ;
      }
      bufLen = newSize ;

   done :
      PD_TRACE_EXITRC ( SDB__MTHDOUBLEBUFFERSIZE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 mthCheckFieldName( const CHAR *pField, INT32 &dollarNum )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pTmp = pField ;
      const CHAR *pDot = NULL ;
      INT32 number = 0 ;
      dollarNum = 0 ;

      while ( pTmp && *pTmp )
      {
         pDot = ossStrchr( pTmp, '.' ) ;
         if ( '$' == *pTmp )
         {
            if ( pDot )
            {
               *(CHAR*)pDot = 0 ;
            }
            rc = ossStrToInt( pTmp + 1, &number ) ;
            // Restore
            if ( pDot )
            {
               *(CHAR*)pDot = '.' ;
            }
            if ( rc )
            {
               goto error ;
            }
            ++dollarNum ;
         }
         pTmp = pDot ? pDot + 1 : NULL ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN mthCheckUnknowDollar( const CHAR *pField,
                                 std::vector<INT64> *dollarList )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pTmp = pField ;
      const CHAR *pDot = NULL ;
      INT32 number = 0 ;
      BOOLEAN hasUnknowDollar = FALSE ;

      while ( pTmp && *pTmp )
      {
         pDot = ossStrchr( pTmp, '.' ) ;

         if ( '$' == *pTmp )
         {
            if ( pDot )
            {
               *(CHAR*)pDot = 0 ;
            }
            rc = ossStrToInt( pTmp + 1, &number ) ;
            // Restore
            if ( pDot )
            {
               *(CHAR*)pDot = '.' ;
            }
            if ( rc )
            {
               goto error ;
            }

            if ( dollarList )
            {
               std::vector<INT64>::iterator it = dollarList->begin() ;
               for ( ; it != dollarList->end() ; ++it )
               {
                  if ( number == (((*it)>>32)&0xFFFFFFFF) )
                  {
                     break ;
                  }
               }
               if ( it == dollarList->end() )
               {
                  goto error ;
               }
            }
         }
         pTmp = pDot ? pDot + 1 : NULL ;
      }

   done:
      return hasUnknowDollar ? FALSE : TRUE ;
   error:
      hasUnknowDollar = TRUE ;
      goto done ;
   }

   INT32 mthConvertSubElemToNumeric( const CHAR *desc,
                                     INT32 &n )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != desc && '$' == *desc, "must be a $" ) ;
      const CHAR *p = desc ;
      const UINT32 maxLen = 10 ;
      CHAR number[maxLen + 1] = { 0 } ;
      UINT32 numberLen = 0 ;

      if ( *p && '$' == *p && p[1] && '[' == p[1] )
      {
         p += 2 ;
         while ( *p )
         {
            if ( '0' <= *p && *p <= '9' )
            {
               if ( numberLen == maxLen )
               {
                  PD_LOG( PDERROR, "number is too long" ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               number[numberLen++] = *p++ ;
            }
            else if ( ']' == *p )
            {
               break ;
            }
            else
            {
               PD_LOG( PDDEBUG, "argument should be a numeric" ) ;
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         if ( 0 == numberLen || ']' != *p )
         {
            PD_LOG( PDDEBUG, "invalid action in selector:%s", desc ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         number[numberLen] = '\0' ;
         n = ossAtoi( number ) ;
      }
      else
      {
         PD_LOG( PDDEBUG, "invalid action:%s", desc ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
     return rc ;
   error:
     goto done ;
   }

   BOOLEAN mthIsModValid( const BSONElement &modmEle )
   {
      if ( modmEle.type() == NumberDecimal )
      {
         bsonDecimal modmDecimal = modmEle.numberDecimal() ;
         if ( modmDecimal.isZero() )
         {
            return FALSE ;
         }
      }
      else if ( modmEle.type() == NumberDouble )
      {
         FLOAT64 f = modmEle.numberDouble() ;
         if ( fabs( f ) <= OSS_EPSILON )
         {
            return FALSE ;
         }
      }
      else
      {
         INT64 modm = modmEle.numberLong() ;
         if ( 0 == modm )
         {
            return FALSE ;
         }
      }

      return TRUE ;
   }

   INT32 _mthAbsBasic( const CHAR *name, const BSONElement &in,
                       BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;

      if ( NumberDouble == in.type() )
      {
         outBuilder.append( name, fabs( in.Double() ) ) ;
      }
      else if ( NumberInt == in.type() )
      {
         INT32 v = in.numberInt() ;
         /// - 2 ^ 31
         if ( OSS_SINT32_MIN != v )
         {
            outBuilder.append( name, 0 <= v ? v : -v ) ;
         }
         else
         {
            outBuilder.append( name, -((INT64)v) ) ;
            flag |= MTH_OPERATION_FLAG_OVERFLOW ;
         }
      }
      else if ( NumberLong == in.type() )
      {
         INT64 v = in.numberLong() ;
         /// return -9223372036854775808 when v is -9223372036854775808
         if ( OSS_SINT64_MIN != v)
         {
            outBuilder.append( name, 0 <= v ? ( INT64 )v : ( INT64 )( -v ) ) ;
         }
         else
         {
            bsonDecimal decResult ;
            rc = decResult.fromString( "9223372036854775808" ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to fromString(\"9223372036854775808\"),rc=%d",
                       rc ) ;
               goto error ;
            }
            outBuilder.append( name, decResult ) ;
            flag |= MTH_OPERATION_FLAG_OVERFLOW ;
         }

      }
      else if ( NumberDecimal == in.type() )
      {
         bsonDecimal decimal ;
         decimal = in.numberDecimal() ;
         rc = decimal.abs() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to ceil decimal:%s,rc=%d",
                    decimal.toString().c_str(), rc ) ;
            goto error ;
         }
         outBuilder.append( name, decimal ) ;
      }
      else if ( !in.eoo() )
      {
         outBuilder.appendNull( name ) ;
      }
      else
      {
         /// do nothing.
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthAbs( const CHAR *name, const BSONElement &in,
                 BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;
      flag = 0 ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthAbsBasic( ele.fieldName(), ele, tmpBuilder, flag ) ;
            PD_CHECK( rc == SDB_OK, rc, error,
                      PDERROR, "failed to Abs:rc=%d", rc ) ;


            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthAbsBasic( name, in, outBuilder, flag ) ;
         PD_CHECK( rc == SDB_OK, rc, error,
                   PDERROR, "failed to Abs:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthCeilingBasic( const CHAR *name, const BSONElement &in,
                           BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( NumberLong == in.type() )
      {
         outBuilder.append( name, ( INT64 )( in.numberLong() ) ) ;
      }
      else if ( NumberInt == in.type() )
      {
         outBuilder.append( name, ( INT32 )( in.numberInt() ) ) ;
      }
      else if ( NumberDouble == in.type() )
      {
         outBuilder.append( name, ( FLOAT64 )ceil( in.numberDouble() ) ) ;
      }
      else if ( NumberDecimal == in.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal result ;
         decimal = in.numberDecimal() ;

         rc = decimal.ceil( result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to ceil decimal:%s,rc=%d",
                    decimal.toString().c_str(), rc ) ;
            goto error ;
         }
         outBuilder.append( name, result ) ;
      }
      else if ( !in.eoo() )
      {
         outBuilder.appendNull( name ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthCeiling( const CHAR *name, const BSONElement &in,
                     BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthCeilingBasic( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to Ceiling:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthCeilingBasic( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to Ceiling:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthFloorBasic( const CHAR *name, const BSONElement &in,
                         BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( NumberInt == in.type() )
      {
         outBuilder.append( name, ( INT32 )( in.numberInt() ) ) ;
      }
      else if ( NumberLong == in.type() )
      {
         outBuilder.append( name, ( INT64 )( in.numberLong() ) ) ;
      }
      else if ( NumberDouble == in.type() )
      {
         outBuilder.append( name, ( FLOAT64 )floor( in.numberDouble() ) ) ;
      }
      else if ( NumberDecimal == in.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal result ;

         decimal = in.numberDecimal() ;
         rc = decimal.floor( result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to floor decimal:%s,rc=%d",
                    decimal.toString().c_str(), rc ) ;
            goto error ;
         }

         outBuilder.append( name, result ) ;
      }
      else if ( !in.eoo() )
      {
         outBuilder.appendNull( name ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthFloor( const CHAR *name, const BSONElement &in,
                   BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthFloorBasic( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to Floor:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthFloorBasic( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to Floor:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthRoundBasic( const CHAR * name, const BSONElement & in,
                         INT32 scale, INT32 &flag,
                         BSONObjBuilder & outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( NumberInt == in.type() )
      {
         if ( scale >= 0 )
         {
            outBuilder.append( name, (INT64)in.numberInt() ) ;
         }
         /// the max value of INT32 is 2147483647, its number of digits is 10
         else if ( -scale <= 9  )
         {
            INT64 i64 = _mthRoundInt( (INT64)in.numberInt(), scale ) ;
            if ( OSS_SINT32_MIN_LL <= i64 && OSS_SINT32_MAX_LL >= i64 )
            {
               outBuilder.append( name, (INT32)i64 ) ;
            }
            else
            {
               outBuilder.append( name, i64 ) ;
               flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
            }
         }
         else
         {
            /// the max value of INT32 is 2147483647, never overflow
            outBuilder.append( name, (INT32)0 ) ;
         }
      }
      else if ( NumberLong == in.type() )
      {
         INT64 value = in.numberLong() ;
         if ( scale >= 0 )
         {
            outBuilder.append( name, value ) ;
         }
         else if ( (UINT32)(-scale) < _mthLog10INT64Size )
         {
            INT64 i = _mthRoundInt( value, scale ) ;
            /* in the range of -4999999999999999999 and 4999999999999999999
             * will never overflow
             */
            if ( MTH_INT64_SAFE_LOW_BOUND <= value &&
                 MTH_INT64_SAFE_UP_BOUND >= value )
            {
               outBuilder.append( name, i ) ;
            }
            else if ( ( i > 0 && value < 0 ) || ( i < 0 && value > 0 ) )
            {
               // overflow
               bsonDecimal decimal ;
               bsonDecimal result ;
               BSONDecimalElement ele( in ) ;

               decimal = ele.numberDecimal() ;
               rc = decimal.round( result, scale ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to round decimal:%s,rc=%d",
                          decimal.toString().c_str(), rc ) ;
                  goto error ;
               }

               outBuilder.append( name, result ) ;
               flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
            }
            else
            {
               outBuilder.append( name, i ) ;
            }
         }
         else if ( -scale == _mthLog10INT64Size )
         {
            /* in some cases:
             * if scale is _mthLog10INT64Size and value is 9223372036854775807,
             * the result should be 1e19 which means overflow.
             * so change INT64 to decimal to return overflow value 1e19.
             */
            if ( MTH_INT64_SAFE_LOW_BOUND > value ||
                 MTH_INT64_SAFE_UP_BOUND < value )
            {
               // overflow
               bsonDecimal decimal ;
               bsonDecimal result ;
               BSONDecimalElement ele( in ) ;

               decimal = ele.numberDecimal() ;
               rc = decimal.round( result, scale ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to round decimal:%s,rc=%d",
                          decimal.toString().c_str(), rc ) ;
                  goto error ;
               }

               outBuilder.append( name, result ) ;
               flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
            }
            /// ohterwise never overflow
            else
            {
               outBuilder.append( name, (INT64)0 ) ;
            }
         }
         /// ohterwise never overflow
         else
         {
            outBuilder.append( name, (INT64)0 ) ;
         }
      }
      else if ( NumberDouble == in.type() )
      {
         BOOLEAN isSpecail = FALSE ;
         FLOAT64 value = in.numberDouble() ;
         FLOAT64 result = _mthRoundFloat( value, scale, isSpecail ) ;
         /* in some cases:
          * if scale is -308 and value is 1.7e308,
          * the result should be 2e308 which means overflow.
          * so change FLOAT64 to decimal to return overflow value 2e308.
          */
         if ( !isSpecail && ( ossIsNaN( result ) || ossIsInf( result ) ) )
         {
            bsonDecimal decimal ;
            bsonDecimal decimalResult ;
            BSONDecimalElement ele( in ) ;

            decimal = ele.numberDecimal() ;
            rc = decimal.round( decimalResult, scale ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to round decimal:%s,rc=%d",
                       decimal.toString().c_str(), rc ) ;
               goto error ;
            }

            outBuilder.append( name, decimalResult ) ;
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }
         /// ohterwise never overflow
         else
         {
            outBuilder.append( name, result ) ;
         }
      }
      else if ( NumberDecimal == in.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal result ;
         BSONDecimalElement ele( in ) ;

         decimal = ele.numberDecimal() ;
         rc = decimal.round( result, scale ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to round decimal:%s,rc=%d",
                    decimal.toString().c_str(), rc ) ;
            goto error ;
         }

         outBuilder.append( name, result ) ;
      }
      else if ( !in.eoo() )
      {
         outBuilder.appendNull( name ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthRound( const CHAR * name, const BSONElement & in,
                   INT32 scale, INT32 &flag,
                   BSONObjBuilder & outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthRoundBasic( ele.fieldName(), ele, scale, flag, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to Round:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthRoundBasic( name, in, scale, flag, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to Round:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthFormatBasic( const CHAR * name, const BSONElement & in,
                          INT32 scale, BSONObjBuilder & outBuilder )
   {
      INT32 rc = SDB_OK ;
      ossPoolString str ;
      switch( in.type() )
      {
         case NumberInt :
         {
            _utilString<UTIL_STRING_INT_LEN+1> us ;
            rc = us.appendINT32( in.numberInt() ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to append int32:%d", rc ) ;
            rc = _mthFormatString( us.str(), scale, str, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to format int32:%d", rc ) ;
            outBuilder.append( name, str ) ;
            break ;
         }
         case NumberLong :
         {
            _utilString<UTIL_STRING_INT64_LEN+1> us ;
            rc = us.appendINT64( in.numberLong() ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to append int64:%d", rc ) ;
            rc = _mthFormatString( us.str(), scale, str, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to format int64:%d", rc ) ;
            outBuilder.append( name, str ) ;
            break ;
         }
         case NumberDouble :
         {
            FLOAT64 f64 = in.numberDouble() ;
            if ( ossIsNaN( f64 ) || ossIsInf( f64 ) )
            {
               _utilString<UTIL_STRING_DOUBLE_LEN+1> us ;
               rc = us.appendDouble( f64 ) ;
               PD_RC_CHECK( rc, PDERROR, "failed to append float64:%d", rc ) ;
               outBuilder.append( name, us.str() ) ;
            }
            else
            {
               bsonDecimal decimal ;
               bsonDecimal tmpDecimal ;

               rc = decimal.fromDouble( f64 ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to parse decimal from double:%g, "
                            "rc:%d", f64, rc ) ;
               rc = decimal.round( tmpDecimal, scale > 0 ? scale : 0 ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to round decimal :%s, "
                            "rc:%d", decimal.toString().c_str(), rc ) ;
               rc = _mthFormatString( tmpDecimal.toString().c_str(),
                                      scale, str, FALSE ) ;
               PD_RC_CHECK( rc, PDERROR, "failed to format float64:%d", rc ) ;
               outBuilder.append( name, str ) ;
            }
            break ;
         }
         case Bool :
            rc = _mthFormatString( in.booleanSafe() ? "1" : "0",
                                   scale, str, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to format boolean:%d", rc ) ;
            outBuilder.append( name, str ) ;
            break ;
         case NumberDecimal :
         {
            BSONDecimalElement ele( in ) ;
            bsonDecimal decimal ;

            decimal = ele.numberDecimal() ;
            if ( decimal.isSpecial() )
            {
               outBuilder.append( name, decimal.toString() ) ;
            }
            else
            {
               bsonDecimal tmpDecimal ;
               rc = decimal.round( tmpDecimal, scale > 0 ? scale : 0 ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to round decimal :%s, "
                            "rc:%d", decimal.toString().c_str(), rc ) ;
               rc = _mthFormatString( tmpDecimal.toString().c_str(),
                                      scale, str, FALSE ) ;
               PD_RC_CHECK( rc, PDERROR, "failed to format decimal:%d", rc ) ;
               outBuilder.append( name, str ) ;
            }
            break ;
         }
         case String :
         {
            bsonDecimal decimal ;
            bsonDecimal tmpDecimal ;

            rc = decimal.fromString( in.String().c_str(), TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse decimal from string:%s, "
                         "rc:%d", in.String().c_str(), rc ) ;

            // if value is specail from String like "NAN", the true value should be 0.
            if ( decimal.isSpecial() )
            {
               decimal.setZero() ;
            }

            rc = decimal.round( tmpDecimal, scale > 0 ? scale : 0 ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to round decimal :%s, "
                         "rc:%d", decimal.toString().c_str(), rc ) ;
            rc = _mthFormatString( tmpDecimal.toString().c_str(), scale, str, FALSE ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to format string:%d", rc ) ;
            outBuilder.append( name, str ) ;
            break ;
         }
         default :
            outBuilder.appendNull( name ) ;
            break ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthFormat( const CHAR * name, const BSONElement & in,
                    INT32 scale, BSONObjBuilder & outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthFormatBasic( ele.fieldName(), ele, scale, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to Format:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthFormatBasic( name, in, scale, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to Format:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthModBasic( const CHAR *name, const BSONElement &in,
                       const BSONElement &modm, BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( in.eoo() )
      {
         /// do nothing.
      }
      else if ( !in.isNumber() || !modm.isNumber() )
      {
         outBuilder.appendNull( name ) ;
      }
      else if ( NumberDecimal == in.type() ||
                NumberDecimal == modm.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;

         decimal    = in.numberDecimal() ;
         decimalArg = modm.numberDecimal() ;
         rc = decimal.mod( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to mod decimal:%s mod %s,rc=%d",
                    decimal.toString().c_str(),
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         outBuilder.append( name, result ) ;
      }
      else if ( FALSE == mthIsModValid( modm ) )
      {
         outBuilder.appendNull( name ) ;
      }
      else if ( NumberDouble == in.type() &&
                NumberDouble == modm.type() )
      {
         FLOAT64 v = MTH_MOD( in.numberDouble(), modm.numberDouble() ) ;
         outBuilder.append( name, v ) ;
      }
      else if ( NumberDouble != in.type () &&
                NumberDouble == modm.type() )
      {
         FLOAT64 v = MTH_MOD( in.numberLong(), modm.numberDouble() ) ;
         outBuilder.append( name, v ) ;
      }
      else if ( NumberDouble == in.type () &&
                NumberDouble != modm.type() )
      {
         FLOAT64 v = MTH_MOD( in.numberDouble(), modm.numberLong() ) ;
         outBuilder.append( name, v ) ;
      }
      else
      {
         INT64 v = in.numberLong() % modm.numberLong() ;
         outBuilder.appendNumber( name, v ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthMod( const CHAR *name, const BSONElement &in,
                 const BSONElement &modm, BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthModBasic( ele.fieldName(), ele, modm, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to Mod:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthModBasic( name, in, modm, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to Mod:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthCastBasic( const CHAR *name, const BSONElement &in,
                        BSONType targetType, BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }

      if ( EOO == targetType )
      {
         PD_LOG( PDERROR, "can not cast to eoo" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( in.type() == targetType )
      {
         outBuilder.appendAs( in, name ) ;
      }
      else
      {
         rc = _mthCast( name, in, targetType, outBuilder ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to cast element[%s] to"
                    " type[%d]", in.toString().c_str(), targetType ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthCast( const CHAR *name, const BSONElement &in,
                  BSONType targetType, BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthCastBasic( ele.fieldName(), ele, targetType, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to Cast:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthCastBasic( name, in, targetType, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to Cast:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthSubStrByByte( const CHAR *name, const BSONElement &in,
                           INT32 begin, INT32 limit, BOOLEAN checkLimit,
                           BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthSubStrByByteBasic( ele.fieldName(), ele, begin, limit,
                                        checkLimit, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to slice substr by byte:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthSubStrByByteBasic( name, in, begin, limit, checkLimit, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to slice substr by byte:rc=%d", rc ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthSubStrByCP( const CHAR *name, const BSONElement &in,
                         INT32 begin, INT32 limit, BOOLEAN checkLimit,
                         BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthSubStrByCPBasic( ele.fieldName(), ele, begin, limit,
                                      checkLimit, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to slice substr by CP:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthSubStrByCPBasic( name, in, begin, limit, checkLimit, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to slice substr by CP:rc=%d", rc ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthSubStrByByteBasic( const CHAR *name, const BSONElement &in,
                                INT32 begin, INT32 limit, BOOLEAN checkLimit,
                                BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         outBuilder.appendNull( name ) ;
      }
      else
      {
         const CHAR *outStr = NULL ;
         INT32 outStrLen    = -1 ;
         _mthGetSubStrByByte( in.valuestr(), in.valuestrsize() - 1, begin, limit, checkLimit,
                              outStr, outStrLen ) ;
         if ( NULL == outStr || -1 == outStrLen )
         {
            outBuilder.append( name, "" ) ;
         }
         else
         {
            outBuilder.appendStrWithNoTerminating( name, outStr, outStrLen ) ;
         }
      }

   done:
      return rc ;
   }

   INT32 _mthSubStrByCPBasic( const CHAR *name, const BSONElement &in,
                              INT32 begin, INT32 limit, BOOLEAN checkLimit,
                              BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         outBuilder.appendNull( name ) ;
      }
      else
      {
         const CHAR *outStr = NULL ;
         INT32 outStrLen    = -1 ;
         _mthGetSubStrByCP( in.valuestr(), in.valuestrsize() - 1, begin, limit, checkLimit,
                            outStr, outStrLen ) ;
         if ( NULL == outStr || -1 == outStrLen )
         {
            outBuilder.append( name, "" ) ;
         }
         else
         {
            outBuilder.appendStrWithNoTerminating( name, outStr, outStrLen ) ;
         }
      }

   done:
      return rc ;
   }

   INT32 _mthConcatBasic( const CHAR *name, const BSONElement &in,
                          const CHAR *prefix, const CHAR *suffix,
                          BOOLEAN isReturnNull, BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( isReturnNull )
      {
         outBuilder.appendNull( name ) ;
      }
      else
      {
         _utilString<> us ;
         rc = us.append( prefix ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append String:%d", rc ) ;
            goto error ;
         }

         rc = mthToString( in, us, isReturnNull ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to convert element to string :%d", rc ) ;
            goto error ;
         }
         else if ( isReturnNull )
         {
            outBuilder.appendNull( name ) ;
            goto done ;
         }

         rc = us.append( suffix ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to append String:%d", rc ) ;
            goto error ;
         }

         outBuilder.append( name, us.str() ) ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthConcat( const CHAR *name, const BSONElement &in,
                    const CHAR *prefix, const CHAR *suffix,
                    BOOLEAN isReturnNull, BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthConcatBasic( ele.fieldName(), ele, prefix, suffix, isReturnNull, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to concat:rc=%d", rc ) ;
            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthConcatBasic( name, in, prefix, suffix, isReturnNull, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to concat:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthGetTime( const BSONElement &e, time_t &timeValue, BOOLEAN &isReturnNull )
   {
      INT32 rc = SDB_OK ;
      switch ( e.type() )
      {
         case Date :
         {
            timeValue = (time_t)( ( INT64 )( e.date() ) / 1000 ) ;
            break ;
         }
         case Timestamp :
         {
            timeValue = (time_t)( ( INT64 )( e.timestampTime() ) / 1000 ) ;
            break ;
         }
         case NumberInt :
         {
            Date_t d( e.numberInt() * 1000LL ) ;
            timeValue = (time_t)( ( INT64 )( d ) / 1000 ) ;
            break ;
         }
         case NumberLong :
         {
            Date_t d( e.numberLong() ) ;
            timeValue = (time_t)( ( INT64 )( d ) / 1000 ) ;
            break ;
         }
         case NumberDouble :
         {
            Date_t d( ossDoubleToINT64( e.numberDouble() ) ) ;
            timeValue = (time_t)( ( INT64 )( d ) / 1000 ) ;
            break ;
         }
         case NumberDecimal :
         {
            bsonDecimal original = e.Decimal() ;
            bsonDecimal l_min ;
            bsonDecimal l_max ;
            rc = l_min.fromLong( OSS_SINT64_MIN ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:rc=%d", rc ) ;
               goto error ;
            }
            rc = l_max.fromLong( OSS_SINT64_MAX ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to parse decimal:rc=%d", rc ) ;
               goto error ;
            }

            if ( original.compare( l_min ) < 0 ||
                  original.compare( l_max ) > 0 )
            {
               isReturnNull = TRUE ;
               goto done ;
            }

            Date_t d( e.numberLong() ) ;
            timeValue = (time_t)( ( INT64 )( d ) / 1000 ) ;
            break ;
         }
         case String :
         {
            UINT64 tm = 0 ;
            if ( SDB_OK == utilStr2Date( e.valuestr(), tm ) )
            {
               Date_t d( tm ) ;
               timeValue = (time_t)( ( INT64 )( d ) / 1000 ) ;
            }
            else
            {
               isReturnNull = TRUE ;
            }
            break ;
         }
         default :
         {
            isReturnNull = TRUE ;
            break ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthDayBasic( const CHAR *name, const BSONElement &in,
                       BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else
      {
         time_t time ;
         BOOLEAN isReturnNull = FALSE ;
         struct tm tm ;
         rc = _mthGetTime( in, time, isReturnNull ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get time:rc=%d", rc ) ;
            goto error ;
         }
         else if ( isReturnNull )
         {
            outBuilder.appendNull( name ) ;
         }
         else
         {
            ossLocalTime( time, tm ) ;
            outBuilder.appendNumber( name, tm.tm_mday ) ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthDay( const CHAR *name, const BSONElement &in,
                 BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthDayBasic( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to mthDayBasic:rc=%d", rc ) ;
            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthDayBasic( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to mthDayBasic:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMonthBasic( const CHAR *name, const BSONElement &in,
                         BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else
      {
         time_t time ;
         BOOLEAN isReturnNull = FALSE ;
         struct tm tm ;
         rc = _mthGetTime( in, time, isReturnNull ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get time:rc=%d", rc ) ;
            goto error ;
         }
         else if ( isReturnNull )
         {
            outBuilder.appendNull( name ) ;
         }
         else
         {
            ossLocalTime( time, tm ) ;
            outBuilder.appendNumber( name, tm.tm_mon + 1 ) ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthMonth( const CHAR *name, const BSONElement &in,
                   BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthMonthBasic( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to mthMonthBasic:rc=%d", rc ) ;
            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthMonthBasic( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to mthMonthBasic:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthYearBasic( const CHAR *name, const BSONElement &in,
                        BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else
      {
         time_t time ;
         BOOLEAN isReturnNull = FALSE ;
         struct tm tm ;
         rc = _mthGetTime( in, time, isReturnNull ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get time:rc=%d", rc ) ;
            goto error ;
         }
         else if ( isReturnNull )
         {
            outBuilder.appendNull( name ) ;
         }
         else
         {
            ossLocalTime( time, tm ) ;
            outBuilder.appendNumber( name, tm.tm_year + 1900 ) ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthYear( const CHAR *name, const BSONElement &in,
                  BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthYearBasic( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to mthYearBasic:rc=%d", rc ) ;
            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthYearBasic( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to mthYearBasic:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthSlice( const CHAR *name, const BSONElement &in,
                   INT32 begin, INT32 limit, BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( Array == in.type() )
      {
         _mthSliceIterator iter( in.embeddedObject(), begin, limit ) ;

         BSONArrayBuilder sliceBuilder( outBuilder.subarrayStart( name ) ) ;
         while ( iter.more() )
         {
            sliceBuilder.append( iter.next() ) ;
         }

         sliceBuilder.doneFast() ;
      }
      else
      {
         outBuilder.append( in ) ;
      }

   done:
      return rc ;
   }

   INT32 mthSubStr( const CHAR *name, const BSONElement &in,
                    INT32 begin, INT32 limit, BSONObjBuilder &outBuilder )
   {
      return _mthSubStrByByte( name, in, begin, limit, FALSE, outBuilder ) ;
   }

   INT32 mthSubStrBytes( const CHAR *name, const BSONElement &in,
                         INT32 begin, INT32 limit, BSONObjBuilder &outBuilder )
   {
      return _mthSubStrByByte( name, in, begin, limit, FALSE, outBuilder ) ;
   }

   INT32 mthSubStrCP( const CHAR *name, const BSONElement &in,
                      INT32 begin, INT32 limit, BSONObjBuilder &outBuilder )
   {
      return _mthSubStrByCP( name, in, begin, limit, FALSE, outBuilder ) ;
   }

   INT32 mthRightBytes( const CHAR *name, const BSONElement &in,
                        INT32 limit, BSONObjBuilder &outBuilder )
   {
      return _mthSubStrByByte( name, in, 0 - limit, limit, TRUE, outBuilder ) ;
   }

   INT32 mthRightCP( const CHAR *name, const BSONElement &in,
                     INT32 limit, BSONObjBuilder &outBuilder )
   {
      return _mthSubStrByCP( name, in,  0 - limit , limit, TRUE, outBuilder ) ;
   }


   INT32 mthLeftBytes( const CHAR *name, const BSONElement &in,
                       INT32 limit, BSONObjBuilder &outBuilder )
   {
      return _mthSubStrByByte( name, in, 0, limit, TRUE, outBuilder ) ;
   }

   INT32 mthLeftCP( const CHAR *name, const BSONElement &in,
                    INT32 limit, BSONObjBuilder &outBuilder )
   {
      return _mthSubStrByCP( name, in, 0, limit, TRUE, outBuilder ) ;
   }


   INT32 _mthStrLenBasic( const CHAR *name, const BSONElement &in,
                          BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         outBuilder.appendNull( name ) ;
      }
      else
      {
         outBuilder.append( name, in.valuestrsize() - 1 ) ;
      }

   done:
      return rc ;
   }

   // UTF-8 encoding rules:
   // single byte: the highest bit is 0
   // multibyte: start from the highest bit, N consecutive bits are all 1. The
   // rest of the bytes all start with 10. N represents the number of encoded
   // bytes
   BOOLEAN _mthIsUTF8StartByte( CHAR charByte )
   {
      return ( charByte & 0xc0 ) != 0x80 ? TRUE : FALSE ;
   }

   INT32 _mthLengthInUTF8CodePoints( const CHAR* str )
   {
      UINT32 i = 0 ;
      INT32 length = 0 ;

      if ( NULL == str )
      {
         goto done ;
      }

      while ( str[i] != '\0' )
      {
         if ( _mthIsUTF8StartByte( str[i] ) )
         {
            length++ ;
         }
         ++i ;
      }

   done:
      return length ;
   }

   INT32 _mthStrlenCP( const CHAR *name, const BSONElement &in,
                       BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         outBuilder.appendNull( name ) ;
      }
      else
      {
         outBuilder.append( name,
                            _mthLengthInUTF8CodePoints ( in.valuestrsafe() ) ) ;
      }

   done:
      return rc ;
   }

   INT32 mthStrLenBytes( const CHAR *name, const BSONElement &in,
                         BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthStrLenBasic( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to StrLen:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthStrLenBasic( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to StrLen:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthStrLen( const CHAR *name, const BSONElement &in,
                    BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      rc = mthStrLenBytes( name, in, outBuilder ) ;
      PD_RC_CHECK( rc, PDERROR, "failed to strlen:rc=%d", rc ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthStrLenCP( const CHAR *name, const BSONElement &in,
                      BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthStrlenCP( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to strlenCP:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthStrlenCP( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to strlenCP:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthLowerBasic( const CHAR *name, const BSONElement &in,
                         BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         outBuilder.appendNull( name ) ;
      }
      else
      {
         _utilString<> us ;
         rc = _lower( in.valuestr(), in.valuestrsize(), us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create lower str:%d", rc ) ;
            goto error ;
         }

         outBuilder.append( name, us.str() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthLower( const CHAR *name, const BSONElement &in,
                   BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthLowerBasic( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to Lower:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthLowerBasic( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to Lower:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthUpperBasic( const CHAR *name, const BSONElement &in,
                         BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         outBuilder.appendNull( name ) ;
      }
      else
      {
         _utilString<> us ;
         rc = _upper( in.valuestr(), in.valuestrsize(), us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create lower str:%d", rc ) ;
            goto error ;
         }

         outBuilder.append( name, us.str() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthUpper( const CHAR *name, const BSONElement &in,
                   BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthUpperBasic( ele.fieldName(), ele, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to Upper:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthUpperBasic( name, in, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to Upper:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN mthIsTrimed( const CHAR *str, INT32 size, INT8 lr )
   {
      BOOLEAN rc = TRUE ;
      SDB_ASSERT( NULL != str, "can not be null" ) ;
      INT32 strLen = 0 <= size ? size : ossStrlen( str ) ;
      if ( 0 == strLen )
      {
         goto done ;
      }

      if ( lr <= 0 )
      {
         if ( ' ' == *str || '\t' == *str || '\n' == *str || '\r' == *str )
         {
            rc = FALSE ;
            goto done ;
         }
      }

      if ( 0 <= lr )
      {
         if ( ' ' == *( str + strLen - 1 ) || '\t' == *( str + strLen - 1 ) ||
              '\n' == *( str + strLen - 1 ) || '\r' == *( str + strLen - 1 ) )
         {
            rc = FALSE ;
            goto done ;
         }
      }

   done:
      return rc ;
   }

   /// lr: -1(ltrim) 0(trim) 1(rtrim)
   INT32 _mthTrimBasic( const CHAR *name, const BSONElement &in, INT8 lr,
                        BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( String != in.type() )
      {
         outBuilder.appendNull( name ) ;
      }
      else if ( mthIsTrimed( in.valuestr(), in.valuestrsize() - 1, lr ) )
      {
         outBuilder.appendAs( in, name ) ;
      }
      else
      {
         _utilString<> us ;
         rc = _mthTrim( in.valuestr(), in.valuestrsize() - 1, lr, us ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to trim string:%d", rc ) ;
            goto error ;
         }

         outBuilder.append( name, us.str() ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /// lr: -1(ltrim) 0(trim) 1(rtrim)
   INT32 mthTrim( const CHAR *name, const BSONElement &in, INT8 lr,
                  BSONObjBuilder &outBuilder )
   {
      INT32 rc = SDB_OK ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthTrimBasic( ele.fieldName(), ele, lr, tmpBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "failed to add trim:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthTrimBasic( name, in, lr, outBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "failed to  trim:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthAddBasic( const CHAR *name, const BSONElement &in,
                       const BSONElement &addend,
                       BSONObjBuilder &outBuilder,
                       INT32 &flag )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( !in.isNumber() )
      {
         outBuilder.appendNull( name ) ;
      }
      else if ( NumberDecimal == in.type() ||
                NumberDecimal == addend.type() )
      {
         bsonDecimal decimalE ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;

         decimalE   = in.numberDecimal() ;
         decimalArg = addend.numberDecimal() ;
         rc = decimalE.add( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add decimal:%s+%s,rc=%d",
                    decimalE.toString().c_str(),
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         outBuilder.append( name, result ) ;
      }
      else if ( NumberDouble == in.type() ||
                NumberDouble == addend.type() )
      {
         FLOAT64 f = addend.numberDouble() + in.numberDouble() ;
         outBuilder.appendNumber( name, f ) ;
      }
      else if ( NumberLong == in.type() ||
                NumberLong == addend.type() )
      {
         INT64 arg1 = addend.numberLong() ;
         INT64 arg2 = in.numberLong() ;
         INT64 i = arg1 + arg2 ;
         if ( utilAddIsOverflow( arg1, arg2, i) )
         {// overflow
            bsonDecimal decimalE ;
            bsonDecimal decimalArg ;
            bsonDecimal result ;

            decimalE   = in.numberDecimal() ;
            decimalArg = addend.numberDecimal() ;
            rc = decimalE.add( decimalArg, result ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to add decimal:%s+%s,rc=%d",
                       decimalE.toString().c_str(),
                       decimalArg.toString().c_str(), rc ) ;
               goto error ;
            }

            outBuilder.append( name, result ) ;
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }
         else if ( NumberInt == in.type() && utilCanConvertToINT32( i ) )
         {
            // keep int if possible
            outBuilder.append( name, (INT32)i ) ;
         }
         else
         {
            outBuilder.append( name, i ) ;
         }

      }
      else // INT32
      {
         INT32 arg1 = addend.numberInt() ;
         INT32 arg2 = in.numberInt() ;
         INT32 i32 = arg1 + arg2 ;
         INT64 i64 = (INT64)arg1 + (INT64)arg2 ;
         if ( (INT64)i32 == i64 )
         {
            outBuilder.append( name, i32 );
         }
         else
         {
            outBuilder.append( name, i64 );
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthAdd( const CHAR *name, const BSONElement &in,
                 const BSONElement &addend,
                 BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;
      flag = 0 ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthAddBasic( ele.fieldName(), ele, addend, tmpBuilder, flag ) ;
            PD_CHECK( rc == SDB_OK, rc, error,
                      PDERROR, "failed to add:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthAddBasic( name, in, addend, outBuilder, flag ) ;
         PD_CHECK( rc == SDB_OK, rc, error,
                   PDERROR, "failed to add:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthSubBasic( const CHAR *name, const BSONElement &in,
                       const BSONElement &subtrahead,
                       BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( !in.isNumber() )
      {
         outBuilder.appendNull( name ) ;
      }
      else if ( NumberDecimal == in.type() ||
                NumberDecimal == subtrahead.type() )
      {
         bsonDecimal decimalE ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;

         decimalE   = in.numberDecimal() ;
         decimalArg = subtrahead.numberDecimal() ;
         rc = decimalE.sub( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to sub decimal:%s-%s,rc=%d",
                    decimalE.toString().c_str(),
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         outBuilder.append( name, result ) ;
      }
      else if ( NumberDouble == in.type() ||
                NumberDouble == subtrahead.type() )
      {
         FLOAT64 f = in.numberDouble() - subtrahead.numberDouble() ;
         outBuilder.appendNumber( name, f ) ;
      }
      else if ( NumberLong == in.type() ||
                NumberLong == subtrahead.type() )
      {
         INT64 arg1 = in.numberLong() ;
         INT64 arg2 = subtrahead.numberLong() ;
         INT64 i = arg1 - arg2 ;
         if ( utilSubIsOverflow( arg1, arg2, i) )
         {// overflow
            bsonDecimal decimalE ;
            bsonDecimal decimalArg ;
            bsonDecimal result ;

            decimalE   = in.numberDecimal() ;
            decimalArg = subtrahead.numberDecimal() ;
            rc = decimalE.sub( decimalArg, result ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to sub decimal:%s+%s,rc=%d",
                       decimalE.toString().c_str(),
                       decimalArg.toString().c_str(), rc ) ;
               goto error ;
            }

            outBuilder.append( name, result ) ;
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }
         else if ( NumberInt == in.type() && utilCanConvertToINT32( i ) )
         {
            outBuilder.append( name, (INT32)i ) ;
         }
         else
         {
            outBuilder.append( name, i ) ;
         }

      }
      else // INT32
      {
         INT32 arg1 = in.numberInt() ;
         INT32 arg2 = subtrahead.numberInt() ;
         INT32 i32 = arg1 - arg2 ;
         INT64 i64 = (INT64)arg1 - (INT64)arg2 ;
         if ( (INT64)i32 == i64 )
         {
            outBuilder.append( name, i32 );
         }
         else
         {
            outBuilder.append( name, i64 );
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthSub( const CHAR *name, const BSONElement &in,
                 const BSONElement &subtrahead,
                 BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;
      flag = 0 ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthSubBasic( ele.fieldName(), ele, subtrahead, tmpBuilder, flag ) ;
            PD_CHECK( rc == SDB_OK, rc, error,
                      PDERROR, "failed to subtract:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthSubBasic( name, in, subtrahead, outBuilder, flag ) ;
         PD_CHECK( rc == SDB_OK, rc, error,
                   PDERROR, "failed to subtract:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthMultiplyBasic( const CHAR *name, const BSONElement &in,
                            const BSONElement &multiplier,
                            BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( !in.isNumber() )
      {
         outBuilder.appendNull( name ) ;
      }
      else if ( NumberDecimal == in.type() ||
                NumberDecimal == multiplier.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;

         decimal    = in.numberDecimal() ;
         decimalArg = multiplier.numberDecimal() ;
         rc = decimal.mul( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to mul decimal:%s*%s,rc=%d",
                    decimal.toString().c_str(),
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         outBuilder.append( name, result ) ;
      }
      else if ( NumberDouble == in.type() ||
                NumberDouble == multiplier.type() )
      {
         FLOAT64 f = multiplier.numberDouble() * in.numberDouble() ;
         outBuilder.appendNumber( name, f ) ;
      }
      else if ( NumberLong == in.type() ||
                NumberLong == multiplier.type() )
      {
         INT64 arg1 = in.numberLong() ;
         INT64 arg2 = multiplier.numberLong() ;
         INT64 i = arg1 * arg2 ;
         if ( utilMulIsOverflow( arg1, arg2, i) )
         {// overflow
            bsonDecimal decimalE ;
            bsonDecimal decimalArg ;
            bsonDecimal result ;

            decimalE   = in.numberDecimal() ;
            decimalArg = multiplier.numberDecimal() ;
            rc = decimalE.mul( decimalArg, result ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to sub decimal:%s+%s,rc=%d",
                       decimalE.toString().c_str(),
                       decimalArg.toString().c_str(), rc ) ;
               goto error ;
            }

            outBuilder.append( name, result ) ;
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }
         else if ( NumberInt == in.type() && utilCanConvertToINT32( i ) )
         {
            // keep int if possible
            outBuilder.append( name, (INT32)i ) ;
         }
         else
         {
            outBuilder.append( name, i ) ;
         }

      }
      else // INT32
      {
         INT32 arg1 = in.numberInt() ;
         INT32 arg2 = multiplier.numberInt() ;
         INT32 i32 = arg1 * arg2 ;
         INT64 i64 = (INT64)arg1 * (INT64)arg2 ;
         if ( (INT64)i32 == i64 )
         {
            outBuilder.append( name, i32 );
         }
         else
         {
            outBuilder.append( name, i64 );
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthMultiply( const CHAR *name, const BSONElement &in,
                      const BSONElement &multiplier,
                      BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;
      flag = 0 ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthMultiplyBasic( ele.fieldName(), ele, multiplier,
                                    tmpBuilder, flag ) ;
            PD_CHECK( rc == SDB_OK, rc, error,
                      PDERROR, "failed to Multiply:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthMultiplyBasic( name, in, multiplier, outBuilder, flag ) ;
         PD_CHECK( rc == SDB_OK, rc, error,
                   PDERROR, "failed to Multiply:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthDivideBasic( const CHAR *name, const BSONElement &in,
                          const BSONElement &divisor,
                          BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;

      if ( in.eoo() )
      {
         goto done ;
      }
      else if ( !in.isNumber() )
      {
         outBuilder.appendNull( name ) ;
      }
      else if ( NumberDecimal == in.type() ||
                NumberDecimal == divisor.type() )
      {
         bsonDecimal decimal ;
         bsonDecimal decimalArg ;
         bsonDecimal result ;

         decimal    = in.numberDecimal() ;
         decimalArg = divisor.numberDecimal() ;
         rc = decimal.div( decimalArg, result ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to div decimal:%s/%s,rc=%d",
                    decimal.toString().c_str(),
                    decimalArg.toString().c_str(), rc ) ;
            goto error ;
         }

         outBuilder.append( name, result ) ;
      }
      else if ( NumberDouble == in.type() ||
                NumberDouble == divisor.type() )
      {
         FLOAT64 r = divisor.numberDouble() ;
         if ( fabs(r) < OSS_EPSILON )
         {
            PD_LOG( PDERROR, "invalid argument:%f", r ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         outBuilder.appendNumber( name, in.numberDouble() / r ) ;
      }
      else if ( NumberLong == in.type() ||
                NumberLong == divisor.type() )
      {
         INT64 divide = in.numberLong() ;
         INT64 r = divisor.numberLong() ;
         INT64 result ;
         if ( 0 == r )
         {
            PD_LOG( PDERROR, "invalid argument:%lld", r ) ;
            rc = SDB_SYS ; /// should not happen. so use sdb_sys.
            goto error ;
         }
         if ( !utilDivIsOverflow( divide, r ) )
         {
            result = divide / r ;
            if ( NumberInt == in.type() && utilCanConvertToINT32( result ) )
            {
               // keep int if possible
               outBuilder.append( name, (INT32)result ) ;
            }
            else
            {
               outBuilder.append( name, result ) ;
            }
         }
         else
         {
            //overflow
            bsonDecimal decResult ;
            rc = decResult.fromString( "9223372036854775808" ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to div decimal:%lld/%lld,rc=%d",
                       divide, r, rc ) ;
               goto error ;
            }
            outBuilder.append( name, decResult ) ;
            flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
         }

      }
      else
      {
         INT32 divide = in.numberInt() ;
         INT32 r = divisor.numberInt() ;
         INT32 result ;
         if ( 0 == r )
         {
            PD_LOG( PDERROR, "invalid argument:%lld", r ) ;
            rc = SDB_SYS ; /// should not happen. so use sdb_sys.
            goto error ;
         }
         if ( -1 == r )
         {
            if ( divide != (INT32)OSS_SINT32_MIN )
            {
               result = -divide ;
               outBuilder.append( name, result ) ;
            }
            else
            {
               INT64 result64 = 2147483648 ;
               outBuilder.append( name, result64 ) ;
               flag |= MTH_OPERATION_FLAG_OVERFLOW ; // overflow
            }
         }
         else
         {
            result = divide / r ;
            outBuilder.append( name, result ) ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthDivide( const CHAR *name, const BSONElement &in,
                    const BSONElement &divisor,
                    BSONObjBuilder &outBuilder, INT32 &flag )
   {
      INT32 rc = SDB_OK ;
      flag = 0 ;
      if ( Array == in.type() )
      {
         BSONArrayBuilder arrayBuilder ;
         BSONObjIterator iter( in.embeddedObject() ) ;
         while ( iter.more() )
         {
            BSONObjBuilder tmpBuilder ;
            BSONElement ele = iter.next() ;
            rc = _mthDivideBasic( ele.fieldName(), ele, divisor, tmpBuilder, flag ) ;
            PD_CHECK( rc == SDB_OK, rc, error,
                      PDERROR, "failed to Divide:rc=%d", rc ) ;

            arrayBuilder.append( tmpBuilder.obj().firstElement() ) ;
         }

         outBuilder.append( name, arrayBuilder.arr() ) ;
      }
      else
      {
         rc = _mthDivideBasic( name, in, divisor, outBuilder, flag ) ;
         PD_CHECK( rc == SDB_OK, rc, error,
                   PDERROR, "failed to Divide:rc=%d", rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthType( const CHAR *name, INT32 outType, const BSONElement &in,
                  BSONObjBuilder &outBuilder )
   {
      if ( !in.eoo() )
      {
         BSONType type = in.type() ;
         if ( 1 == outType )
         {
            outBuilder.append( name, type ) ;
         }
         else
         {
            string typeName = "" ;
            mthGetCastTranslator()->getCastStr( type, typeName ) ;
            outBuilder.append( name, typeName ) ;
         }
      }

      return SDB_OK ;
   }

   INT32 mthIfNull( const CHAR *name, const BSONElement &in,
                    const BSONElement &value,
                    BSONObjBuilder &outBuilder )
   {
      if ( in.eoo() || in.isNull() || Undefined == in.type() )
      {
         outBuilder.appendAs( value, name ) ;
      }
      else
      {
         outBuilder.appendAs( in, name ) ;
      }

      return SDB_OK ;
   }

   INT32 mthSize( const CHAR *name, const BSONElement &in,
                  BSONObjBuilder &outBuilder )
   {
      if ( in.eoo() )
      {
         goto done ;
      }

      if ( in.type() == Array || in.type() == Object )
      {
         outBuilder.append( name, in.embeddedObject().nFields() ) ;
      }
      else
      {
         outBuilder.appendNull( name ) ;
      }

   done:
      return SDB_OK ;
   }

   _mthCastTranslator::_mthCastTranslator()
   {
      INT32 i   = 0 ;
      INT32 len = 0 ;

      len = sizeof( g_cast_str_to_type_array) / sizeof( mthCastStr2Type ) ;
      for ( i = 0 ; i < len ; i++ )
      {
         mthCastStr2Type *ptype = &g_cast_str_to_type_array[i] ;
         _castTransMap[ ptype->castStr ] = ptype->castType ;
         _castTypeMap[ ptype->castType ] = ptype->castStr ;
      }
   }

   _mthCastTranslator::~_mthCastTranslator()
   {
      _castTransMap.clear() ;
      _castTypeMap.clear() ;
   }

   INT32 _mthCastTranslator::getCastType( const CHAR *typeStr, BSONType &type )
   {
      MTH_CAST_NAME_MAP::iterator iter ;

      INT32 rc = SDB_OK ;
      _utilString<20> us ;
      const CHAR *p = typeStr ;
      while ( '\0' != *p )
      {
         if ( 'A' <= *p && *p <= 'Z' )
         {
            rc = us.append( *p + 32 ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "append str failed:str=%s,rc=%d",
                       typeStr, rc ) ;
               goto error ;
            }
         }
         else
         {
            rc = us.append( *p ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "append str failed:str=%s,rc=%d",
                       typeStr, rc ) ;
               goto error ;
            }
         }

         ++p ;
      }

      iter = _castTransMap.find( us.str() ) ;
      if ( iter == _castTransMap.end() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "unknown type:typeStr=%s", typeStr ) ;
         goto error ;
      }

      type = iter->second ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _mthCastTranslator::getCastStr( BSONType type, string &name )
   {
      MTH_CAST_TYPE_MAP::iterator iter ;
      iter = _castTypeMap.find( type ) ;
      if ( iter == _castTypeMap.end() )
      {
         name = "Unknown Type" ;
      }
      else
      {
         name = iter->second ;
      }

      return SDB_OK ;
   }

   _mthCastTranslator *mthGetCastTranslator()
   {
      static _mthCastTranslator translator ;

      return &translator ;
   }

   _mthSliceIterator::_mthSliceIterator( const bson::BSONObj &obj, INT32 begin,
                                         INT32 limit )
   :_obj( obj ), _where( 0 ), _limit( limit ), _itr( _obj )
   {
      INT32 total = obj.nFields() ;
      _where = begin < 0 ? begin + total : begin ;
      if ( _where < 0 )
      {
         _where = 0 ;
      }

      while ( 0 != _where )
      {
         if ( _itr.more() )
         {
            _itr.next() ;
            --_where ;
         }
         else
         {
            _limit = 0 ;
            break ;
         }
      }
   }

   _mthSliceIterator::~_mthSliceIterator()
   {
   }

   BOOLEAN _mthSliceIterator::more()
   {
      return _limit != 0 && _itr.more() ;
   }

   bson::BSONElement _mthSliceIterator::next()
   {
      if ( more() )
      {
         if ( 0 < _limit )
         {
            --_limit ;
         }

         return _itr.next() ;
      }
      else
      {
         return BSONElement() ;
      }
   }

   BOOLEAN mthIsNumber1( const bson::BSONElement &ele )
   {
      if ( ele.isNumber() )
      {
         if ( ele.numberInt() == 1 )
         {
            return TRUE ;
         }
      }

      return FALSE ;
   }

   //substr[begin, len]/slice[begin, len]  len=-1 means unlimit len
   BOOLEAN mthIsValidLen( INT32 length )
   {
      return TRUE ;
   }

   INT32 mthCheckIfSubFieldIsOp( const BSONElement &ele, BOOLEAN &subFieldIsOp )
   {
      INT32 rc = SDB_OK ;
      subFieldIsOp = FALSE ;

      try
      {
         BSONObjIterator itr( ele.embeddedObject() ) ;
         while ( itr.more() )
         {
            BSONElement e = itr.next() ;
            const CHAR *fieldName = e.fieldName() ;

            if ( MTH_OPERATOR_EYECATCHER != fieldName[0] ||
                 0 == ossStrcmp( fieldName, MTH_OPERATOR_STR_AND ) ||
                 0 == ossStrcmp( fieldName, MTH_OPERATOR_STR_OR ) ||
                 0 == ossStrcmp( fieldName, MTH_OPERATOR_STR_NOT ) )
            {
               continue ;
            }
            else
            {
               subFieldIsOp = TRUE ;
               break ;
            }
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG ( PDERROR, "Check if the subfield name is an operator name"
                  "exception: %s, rc: %d", e.what(), rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthParseSubStrArgs( const bson::BSONElement &e, BOOLEAN allowToArgs,
                             INT32 &begin, INT32 &limit )
   {
      INT32 rc = SDB_OK ;

      if ( e.isNumber() && 0 <= e.numberInt() )
      {
         limit = e.numberInt() ;
      }
      else if ( e.isNumber() )
      {
         begin = e.numberInt() ;
      }
      else if ( Array == e.type() )
      {
         BSONObjIterator i( e.embeddedObject() ) ;
         BSONElement ele ;

         if ( !i.more() || !allowToArgs )
         {
            goto invalid_arg ;
         }

         ele = i.next() ;
         if ( !ele.isNumber() )
         {
            goto invalid_arg ;
         }

         begin = ele.numberInt() ;

         if ( !i.more() )
         {
            goto invalid_arg ;
         }

         ele = i.next() ;
         if ( !ele.isNumber() )
         {
            goto invalid_arg ;
         }

         limit = ele.numberInt() ;

         if ( i.more() )
         {
            goto invalid_arg ;
         }
      }
      else
      {
         PD_LOG( PDERROR, "invalid element" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   invalid_arg:
      PD_LOG( PDERROR, "invalid argument:%s",
              e.toString( TRUE, TRUE ).c_str() ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   static INT32 _mthConcatArrayToStr( const bson::BSONElement &e,
                                      INT32 pos, BOOLEAN &isReturnNull,
                                      _utilString<> &prefix, _utilString<> &suffix )
   {
      INT32 rc = SDB_OK ;

      BSONObj obj = e.embeddedObject() ;
      BSONObjIterator itr( obj ) ;
      INT32 curPos = 0 ;
      INT32 splitPos = pos < 0 ? pos + obj.nFields() + 1 : pos ;
      // pos over left boundary
      splitPos = splitPos < 0 ? 0 : splitPos ;
      while ( itr.more() )
      {
         BSONElement ele = itr.next() ;
         if ( curPos < splitPos )
         {
            rc = mthToString( ele, prefix, isReturnNull ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to convert element to string :%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            rc = mthToString( ele, suffix, isReturnNull ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to convert element to string :%d", rc ) ;
               goto error ;
            }
         }
         // if args cannot be converted to str, just exit
         if ( isReturnNull )
         {
            break ;
         }
         ++ curPos ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 mthParseConcatArrayArgs( const bson::BSONElement &e, BOOLEAN &isReturnNull,
                                  _utilString<> &prefix, _utilString<> &suffix )
   {
      INT32 rc = SDB_OK ;
      INT32 pos = 0 ;
      BSONObjIterator i( e.embeddedObject() ) ;
      BSONElement ele ;
      if ( !i.more() )
      {
         goto invalid_arg ;
      }

      // the first arg must be number
      ele = i.next() ;
      if ( !ele.isNumber() )
      {
         goto invalid_arg ;
      }
      pos = ele.numberInt() ;

      if ( !i.more() )
      {
         goto invalid_arg ;
      }

      // the second arg must be array
      ele = i.next() ;
      if ( Array != ele.type() )
      {
         goto invalid_arg ;
      }

      rc = _mthConcatArrayToStr( ele, pos, isReturnNull, prefix, suffix ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to Convert array elements to str rc = %d", rc ) ;
         goto error ;
      }

      if ( i.more() )
      {
         goto invalid_arg ;
      }

   done:
      return rc ;
   error:
      goto done ;
   invalid_arg:
      PD_LOG( PDERROR, "invalid argument:%s",
              e.toString( TRUE, TRUE ).c_str() ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }


   INT32 mthToString( const bson::BSONElement &e, _utilString<> &us,
                      BOOLEAN &isReturnNull )
   {
      INT32 rc = SDB_OK ;

      switch ( e.type() )
      {
         case MinKey :
         case EOO :
         case BinData :
         case Undefined :
         case jstNULL :
         case RegEx :
         case DBRef :
         case CodeWScope :
         case Symbol :
         case Code :
         case MaxKey :
            isReturnNull = TRUE ;
            break ;
         case String :
         {
            rc = us.append( e.valuestr() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append String:%d", rc ) ;
               goto error ;
            }
            break ;
         }
         case NumberInt :
         {
            rc = us.appendINT32( e.numberInt() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append int32:%d", rc ) ;
               goto error ;
            }
            break ;
         }
         case NumberLong :
         {
            rc = us.appendINT64( e.numberLong() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append int64:%d", rc ) ;
               goto error ;
            }
            break ;
         }
         case NumberDouble :
         {
            rc = us.appendDouble( e.numberDouble() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append float64:%d", rc ) ;
               goto error ;
            }
            break ;
         }
         case NumberDecimal :
         {
            rc = us.appendDecimal( e.numberDecimal() );
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append decimal %d", rc ) ;
               goto error ;
            }
            break ;
         }
         case Date :
         {
            rc = us.appendDate( e.date() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append Date:%d", rc ) ;
               goto error ;
            }
            break ;
         }
         case Timestamp :
         {
            rc = us.appendTimestamp( e.timestampTime(), e.timestampInc() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append Timestamp %d", rc ) ;
               goto error ;
            }
            break ;
         }
         case jstOID :
         {
            rc = us.appendOID( e.OID() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append OID:%d", rc ) ;
               goto error ;
            }
            break ;
         }
         case Object :
         {
            rc = us.append( e.embeddedObject().toString( FALSE, TRUE ).c_str() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append OID:%d", rc ) ;
               goto error ;
            }
            break ;
         }
         case Array :
         {
            rc = us.append( e.embeddedObject().toString( TRUE, TRUE ).c_str() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append Array:%d", rc ) ;
               goto error ;
            }
            break ;
         }
         case Bool :
         {
            rc = us.appendBool( e.booleanSafe() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append Bool:%d", rc ) ;
               goto error ;
            }
            break ;
         }
         default:
            isReturnNull = TRUE ;
            break ;
      }
   done:
      return rc ;
   error:
      goto done ;

   }

   INT32 mthMatchGT( const BSONElement &left,
                     const BSONElement &right,
                     BOOLEAN mixCmp,
                     BOOLEAN rejectArray,
                     BOOLEAN &result )
   {
      // Special cases for minKey and maxKey
      if ( right.canonicalType() == MinKey )
      {
         // $gt:$minKey returns everything except for $minKey
         result = ( left.canonicalType() != MinKey ) ;
         return SDB_OK ;
      }
      else if ( right.canonicalType() == MaxKey )
      {
         // $gt:$maxKey returns nothing
         result = FALSE ;
         return SDB_OK ;
      }

      // If in mix-compare mode, left and right could have different canonical
      // types, otherwise, they should have the same canonical types
      if ( left.canonicalType() == right.canonicalType() )
      {
         if ( compareElementValues ( left, right ) > 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }
      else if ( mixCmp )
      {
         if ( rejectArray && left.type() == Array && right.type() != Array )
         {
            // Let the caller split array
            result = FALSE ;
            return SDB_OK ;
         }
         if ( left.woCompare( right, FALSE ) > 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }

      result = FALSE ;
      return SDB_OK ;
   }

   INT32 mthMatchGTE( const BSONElement &left,
                      const BSONElement &right,
                      BOOLEAN mixCmp,
                      BOOLEAN rejectArray,
                      BOOLEAN &result )
   {
      // Special cases for minKey and maxKey
      if ( right.canonicalType() == MinKey )
      {
         // $gte:$minKey returns everything
         result = TRUE ;
         return SDB_OK ;
      }
      else if ( right.canonicalType() == MaxKey )
      {
         // $gte:$maxKey returns $maxKey
         result = ( left.canonicalType() == MaxKey ) ;
         return SDB_OK ;
      }

      // If in mix-compare mode, left and right could have different canonical
      // types, otherwise, they should have the same canonical types
      if ( left.canonicalType() == right.canonicalType() )
      {
         if ( compareElementValues ( left, right ) >= 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }
      else if ( mixCmp )
      {
         if ( rejectArray && left.type() == Array && right.type() != Array )
         {
            // Let the caller split array
            result = FALSE ;
            return SDB_OK ;
         }
         if ( left.woCompare( right, FALSE ) >= 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }

      result = FALSE ;
      return SDB_OK ;
   }

   INT32 mthMatchLT( const BSONElement &left,
                     const BSONElement &right,
                     BOOLEAN mixCmp,
                     BOOLEAN rejectArray,
                     BOOLEAN &result )
   {
      // Special cases for minKey and maxKey
      if ( right.canonicalType() == MinKey )
      {
         // $lt:$minKey returns nothing
         result = FALSE ;
         return SDB_OK ;
      }
      else if ( right.canonicalType() == MaxKey )
      {
         // $lt:$maxKey returns everything except for $maxKey
         result = ( left.canonicalType() != MaxKey ) ;
         return SDB_OK ;
      }

      // If in mix-compare mode, left and right could have different canonical
      // types, otherwise, they should have the same canonical types
      if ( left.canonicalType() == right.canonicalType() )
      {
         if ( compareElementValues ( left, right ) < 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }
      else if ( mixCmp )
      {
         if ( rejectArray && left.type() == Array && right.type() != Array )
         {
            // Let the caller split array
            result = FALSE ;
            return SDB_OK ;
         }
         if ( left.woCompare( right, FALSE ) < 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }

      result = FALSE ;
      return SDB_OK ;
   }

   INT32 mthMatchLTE( const BSONElement &left,
                      const BSONElement &right,
                      BOOLEAN mixCmp,
                      BOOLEAN rejectArray,
                      BOOLEAN &result )
   {
      // Special cases for minKey and maxKey
      if ( right.canonicalType() == MinKey )
      {
         // $lte:$minKey returns only $minKey
         result = ( left.canonicalType() == MinKey ) ;
         return SDB_OK ;
      }
      else if ( right.canonicalType() == MaxKey )
      {
         // $lte:$maxKey returns everything
         result = TRUE ;
         return SDB_OK ;
      }

      // If in mix-compare mode, left and right could have different canonical
      // types, otherwise, they should have the same canonical types
      if ( left.canonicalType() == right.canonicalType() )
      {
         if ( compareElementValues ( left, right ) <= 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }
      else if ( mixCmp )
      {
         if ( rejectArray && left.type() == Array && right.type() != Array )
         {
            // Let the caller split array
            result = FALSE ;
            return SDB_OK ;
         }
         if ( left.woCompare( right, FALSE ) <= 0 )
         {
            result = TRUE ;
            return SDB_OK ;
         }
      }

      result = FALSE ;
      return SDB_OK ;
   }

}


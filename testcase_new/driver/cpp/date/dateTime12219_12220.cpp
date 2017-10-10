/*************************************************************************
 * @Description: test case for Jira questionaire
 *				     SEQUOIADBMAINSTREAM-1068
 *               seqDB-12219:构造sdbDate数据并使用toString方法转为字符串
 *               seqDB-12220:构造Timestamp数据并使用toString方法转为字符串
 * @Modify:      Liang xuewang Init
 *				     2017-07-20
 *************************************************************************/
#include <client.hpp>
#include <gtest/gtest.h>
#include <iostream>
#include "testcommon.hpp" 

using namespace sdbclient ;
using namespace bson ;
using namespace std ;

TEST( dateTest12219, date12219 )
{
   SINT64 mills[] = {
      -62167248352000,   // 0000-01-01 00:00:00
      253402271999000,   // 9999-12-31 23:59:59
      -62167248353000,   // < 0000-01-01 00:00:00 
      253402272000000,   // > 9999-12-31 23:59:59
      -9223372036854775808,  // -2^63
      9223372036854775807    // 2^63-1
   } ;
   INT32 size = sizeof(mills) / sizeof(mills[0]) ;
   BSONObjBuilder builder ;
   for( INT32 i = 0;i < size;i++ )
   {
      CHAR key[10] ;
      sprintf( key, "%s%d", "date", i ) ;
      Date_t dt( mills[i] ) ;
      builder.appendDate( key, dt ) ;
   }
   BSONObj obj = builder.obj() ;
   // 夏令时、时区导致可能出现两种结果
   const CHAR* expect1 = "{ \"date0\": {\"$date\": \"0000-01-01\"}, "
                           "\"date1\": {\"$date\": \"9999-12-31\"}, "
                           "\"date2\": { \"$date\": -62167248353000 }, "
                           "\"date3\": { \"$date\": 253402272000000 }, "
                           "\"date4\": { \"$date\": -9223372036854775808 }, "
                           "\"date5\": { \"$date\": 9223372036854775807 } }" ;
   const CHAR* expect2 = "{ \"date0\": { \"$date\": -62167248352000 }, "
                           "\"date1\": {\"$date\": \"9999-12-31\"}, "
                           "\"date2\": { \"$date\": -62167248353000 }, "
                           "\"date3\": { \"$date\": 253402272000000 }, "
                           "\"date4\": { \"$date\": -9223372036854775808 }, "
                           "\"date5\": { \"$date\": 9223372036854775807 } }" ;
   string tmp = obj.toString() ;
   const CHAR* real = tmp.c_str() ;
   ASSERT_TRUE( !strcmp( expect1, real ) || !strcmp( expect2, real ) ) 
                << "fail to check date\n expect1 = " << expect1 
                << "\n expect2 = " << expect2 << "\n real = " << real ; 
}

TEST( timestampTest12220, timestamp12220 )
{
   INT32 secs[] = {
      -2145945600,   	// 1902-01-01 00:00:00.000000
      -1325491200,     // 1928-01-01 00:00:00.000000
      2145887999     	// 2037-12-31 23:59:59.999999
   } ;
   INT32 micros[] = {
      0,  			// micro seconds for 1902-01-01 00:00:00.000000
      0,              // micro seconds for 1928-01-01 00:00:00.000000
      999999 			// micro seconds for 2037-12-31 23:59:59.999999
   } ;
   INT32 size = sizeof(secs) / sizeof(secs[0]) ;
   BSONObjBuilder builder ;
   for( INT32 i = 0;i < size;i++ )
   {
      CHAR key[10] ;
      sprintf( key, "%s%d", "time", i ) ;
      UINT64 tmp = 0 ;
      memcpy( (CHAR*)&tmp, &micros[i], sizeof(INT32)) ;
      memcpy( (CHAR*)&tmp + 4, &secs[i], sizeof(INT32)) ;
      builder.appendTimestamp( key, tmp ) ;
   }
   BSONObj obj = builder.obj() ;
   string tmp = obj.toString() ;
   const CHAR* real = tmp.c_str() ;
   // 夏令时、时区导致可能出现两种结果
   const CHAR* expect1 = "{ \"time0\": {\"$timestamp\": \"1902-01-01-00.05.52.000000\"}, "
                           "\"time1\": {\"$timestamp\": \"1928-01-01-00.00.00.000000\"}, "
                           "\"time2\": {\"$timestamp\": \"2037-12-31-23.59.59.999999\"} }" ;
   const char* expect2 = "{ \"time0\": {\"$timestamp\": \"1902-01-01-00.00.00.000000\"}, "
                           "\"time1\": {\"$timestamp\": \"1928-01-01-00.00.00.000000\"}, "
                           "\"time2\": {\"$timestamp\": \"2037-12-31-23.59.59.999999\"} }" ;	
   ASSERT_TRUE( !strcmp( expect1, real ) || !strcmp( expect2, real ) ) 
                << "fail to check timestamp\n expect1 = " << expect1 
                << "\n expect2 = " << expect2 << "\n real = " << real ;
}

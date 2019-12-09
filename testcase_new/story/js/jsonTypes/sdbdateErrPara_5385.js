/******************************************************************************
*@Description : wrong parameter test SdbDate function
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

function main ()
{
   // 测试SdbDate函数参数不正确的错误
   // SdbDate()当前日期
   // SdbDate( "YYYY-MM-DD" )指定日期
   var ErrPara = [["2016-07-11", 1],
   ["2016-07"]];
   var ErrCode = -6;
   for( var i = 0; i < ErrPara.length; ++i )
   {
      try
      {
         SdbDate( ErrPara[i] );
      }
      catch( e )
      {
         if( e == ErrCode )
            println( ">success to test sdbdate with wrong parameters." );
         else
         {
            println( ">fail to test sdbdate with wrong parameters." );
            throw ( e );
         }
      }
   }

   // SdbDate( 2016-07-11 )= SdbDate( "1998" )2016-7-11 = 1998 会正常返回，需要优化改进
}


// Test
try
{
   main();
}
catch( e )
{
   throw e;
}

/******************************************************************************
*@Description : test NumberLong function with Boundary Value
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

function main ()
{
   // 测试NumberLong函数参数为边界值的情况
   // NumberLong( 100 ) NumberLong( "100" )指定64位整数
   var minv = -9007199254740992;
   var maxv = 9007199254740992;

   if( NumberLong( minv ) == minv )
      println( ">success to test numberlong with low boundary value." );
   else
      throw ( ">fail to test numberlong with low boundary value." );

   if( NumberLong( maxv ) == maxv )
      println( ">success to test numberlong with high boundary value." );
   else
      throw ( ">fail to test numberlong with high boundary value." );
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

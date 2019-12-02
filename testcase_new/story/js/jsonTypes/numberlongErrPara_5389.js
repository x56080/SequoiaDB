/******************************************************************************
*@Description : wrong parameter test NumberLong function
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

function main()
{
   // 测试NumberLong函数参数不正确的错误
   // NumberLong( 100 ) NumberLong( "100" )指定64位整数
   var ErrPara = [ [100, 101], ["100", 101], [], 
   [true], ["abc"] ]; 
   var ErrCode = -6; 
   for( var i = 0; i < ErrPara.length; ++i )
   {
      try
      {
         NumberLong( ErrPara[i] ); 
      }
      catch( e )
      {
         if( e == ErrCode )
         println( ">success to test numberlong with wrong parameters." ); 
         else
         {
            println( ">fail to test numberlong with wrong parameters." ); 
            throw( e ); 
         }
      }
   }
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

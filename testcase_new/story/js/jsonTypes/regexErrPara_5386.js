/******************************************************************************
*@Description : wrong parameter test Regex function
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

function main()
{
   // 测试Regex函数参数不正确的错误
   // Regex( "表达式", "类型" )指定正则表达式
   var ErrPara = [ ["^W"] ]; 
   var ErrCode = -6; 
   for( var i = 0; i < ErrPara.length; ++i )
   {
      try
      {
         Regex( ErrPara[i] ); 
      }
      catch( e )
      {
         if( e == ErrCode )
         println( ">success to test regex with wrong parameters." ); 
         else
         {
            println( ">fail to test regex with wrong parameters." ); 
            throw( e ); 
         }
      }
   }
   
   var rg = Regex( "^W", "i", "i" ); 
   if( rg.toString()== Regex( "^W", "i" ).toString() )
   println( ">success to test regex with more parameter." ); 
   else
   throw( ">fail to test regex with more parameter." ); 
   
   // Regex( "^W", i ) Regex( W, "i" )报语法错误，跳过测试
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

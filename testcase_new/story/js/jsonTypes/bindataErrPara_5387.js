/******************************************************************************
*@Description : wrong parameter test BinData function
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

function main ()
{
   // 测试Bindata函数参数不正确的错误
   // BinData( "base64加密后的内容", "类型" )指定二进制对象
   var ErrPara = [["aGVsbG8gd29ybGQ=", "1", "1"], ["aGVsbG8gd29ybGQ="]];
   var ErrCode = -6;
   for( var i = 0; i < ErrPara.length; ++i )
   {
      try
      {
         BinData( ErrPara[i] );
      }
      catch( e )
      {
         if( e == ErrCode )
            println( ">success to test bindata with wrong parameters." );
         else
         {
            println( ">fail to test bindata with wrong parameters." );
            throw ( e );
         }
      }
   }

   var bd = BinData( "aGVsbG8gd29ybGQ=", 1 );
   if( bd.toString() == BinData( "aGVsbG8gd29ybGQ=", "1" ).toString() )
      println( ">success to test bindata with second parameter as a number." );
   else
      throw ( ">fail to test bindata with second parameter as a number." );

   // BinData( aGVsbG8gd29ybGQ =, "1" )报语法错误，跳过测试
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

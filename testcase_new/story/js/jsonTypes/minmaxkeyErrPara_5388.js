/******************************************************************************
*@Description : wrong parameter test MinKey MaxKey function
*@Modify list :
*               2016-07-11   XueWang Liang  Init
******************************************************************************/

function main ()
{
   // 测试MinKey MaxKey函数参数不正确的错误
   // MinKey() MaxKey()生成MinKey, MaxKey对象
   var ErrPara = [[1], ["1", "1"]];
   var mink = MinKey();
   var maxk = MaxKey();
   var temp;

   for( var i = 0; i < ErrPara.length; ++i )
   {
      try
      {
         temp = MinKey( ErrPara[i] );
      }
      catch( e )
      {
         if( -6 == e )
            println( ">success to test minkey with wrong parameter." );
         else
            throw ( ">fail to test minkey with wrong parameter." );
      }
      try
      {
         temp = MaxKey( ErrPara[i] );
      }
      catch( e )
      {
         if( -6 == e )
            println( ">success to test maxkey with wrong parameter." );
         else
            throw ( ">fail to test maxkey with wrong parameter." );
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

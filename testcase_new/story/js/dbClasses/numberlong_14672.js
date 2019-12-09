/*******************************************************************
* @Description : test case for NumberLong
*                seqDB-14672:使用valueOf获取NumberLong值
* @author      : Liang XueWang
*                2018-03-12
*******************************************************************/
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}
;

function main ()
{
   var number = 2147483648;
   var numberLong = NumberLong( number );
   if( numberLong.valueOf() !== number )
   {
      throw new Error( "check valueOf, expect: " + number + ", actual: " + numberLong.valueOf() );
   }
}

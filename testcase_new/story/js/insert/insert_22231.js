/************************************
*@Description: seqDB-22231:插入错误$date参数报错_insert({a:{$date:null}})
*@Author      : 2020.06.01:chimanzhao
**************************************/
import( "../lib/main.js" );
testConf.clName = COMMCLNAME + "_22231";

main( test )

function test ( testPara )
{
   var dbcl = testPara.testCL;
   try
   {
      dbcl.insert( { a: { $date: null } } )
   }
   catch( e )
   {
      if( e.message != -6 )
      {
         throw e;
      }
   }
}
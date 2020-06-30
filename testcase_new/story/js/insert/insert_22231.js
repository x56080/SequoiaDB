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
      assert.fail( "exp fail but act success!" )
   }
   catch( e )
   {
      if( parseInt( e.message ) !== -6 )
      {
         throw new Error( e );
      }
   }
}

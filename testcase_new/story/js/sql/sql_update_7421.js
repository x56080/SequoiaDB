/****************************************************
@description:	update by SQL, basic case
         testlink cases:   seqDB-7421
@input:        1 insert into records
               2 update without condition
               3 update one record with condition
               4 update multiple records with condition
@output:    return success, and results correct.
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
testConf.csName = COMMCSNAME, testConf.csOpt = { PageSize: 4096 };
testConf.clName = CHANGEDPREFIX + "_7421", testConf.clOpt = { ReplSize: 0 };

main( test );

function test ()
{

   testPara.testCL.insert( { name: "Tom", age: 1 } );
   testPara.testCL.insert( { name: "Mike", age: 2 } );
   testPara.testCL.insert( { name: "Lisa", age: 3 } );
   testPara.testCL.insert( { name: "Json", age: 4 } );
   testPara.testCL.insert( { name: "Jhon", age: 5 } );
   testPara.testCL.insert( { name: "Tina", age: 6 } );
   testPara.testCL.insert( { name: "Pite", age: 7 } );

   db.execUpdate( "update " + testConf.csName + "." + testConf.clName + " set phone=123" );

   var rc = testPara.testCL.find();
   if( 7 != rc.size() )
   {
      throw new Error( "Failed to compare results." );
   }

   db.execUpdate( "update " + testConf.csName + "." + testConf.clName + " set age=10 where name=\"Lisa\"" );

   var rc = testPara.testCL.find( { name: "Lisa" } );
   while( rc.next() )
   {
      if( 10 != rc.current().toObj()["age"] )
      {
         throw new Error( "Failed to compare results." );
      }
   }

   db.execUpdate( "update " + testConf.csName + "." + testConf.clName + " set phone=456 where age>4" );

   var rc = testPara.testCL.find( { age: { $gt: 4 } } );
   while( rc.next() )
   {
      if( 456 != rc.current().toObj()["phone"] )
      {
         throw new Error( "Failed to compare results." );
      }
   }
}
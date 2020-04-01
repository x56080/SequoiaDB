/****************************************************
@description:	select with [count()] by SQL, basic case
         testlink cases:   seqDB-7437
@input:        1 insert into records
               2 select with [count()]
@output:    return success, and results correct.
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
testConf.csName = COMMCSNAME, testConf.csOpt = { PageSize: 4096 };
testConf.clName = CHANGEDPREFIX + "_7437", testConf.clOpt = { ReplSize: 0 };

main( test );

function test ()
{
   testPara.testCL.insert( { name: "Tom", age: 20 } );
   testPara.testCL.insert( { name: "Tom", age: 20 } );
   testPara.testCL.insert( { name: "Miko", age: 30 } );
   testPara.testCL.insert( { name: "Jhon", age: 10 } );

   var rc;
   try
   {
      rc = db.exec( "select count(name) as count_name from " + testConf.csName + "." + testConf.clName );
   }
   catch( e )
   {
      throw new Error( "Failed to select with [count()]" );
   }

   if( 4 != rc.current().toObj()["count_name"] )
   {
      throw new Error( "Failed to check results." );
   }
}
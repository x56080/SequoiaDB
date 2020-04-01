/****************************************************
@description:	select with [max()] by SQL, basic case
         testlink cases:   seqDB-7439
@input:        1 insert into records
               2 
@output:    return success, and results correct.
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
testConf.csName = COMMCSNAME, testConf.csOpt = { PageSize: 4096 };
testConf.clName = CHANGEDPREFIX + "_7439", testConf.clOpt = { ReplSize: 0 };

main( test );

function test ()
{
   testPara.testCL.insert( { name: "Tom", age: 20 } );
   testPara.testCL.insert( { name: "Tomi", age: 70 } );

   var rc = db.exec( "select max(age) as max_age from " + testConf.csName + "." + testConf.clName );

   if( 70 !== rc.current().toObj()["max_age"] )
   {
      throw new Error( "Failed to check results." );
   }
}

/*******************************************************************************
*@Description : [seqDB-13755] when run 'db.default.tt.find().sort({"":"a"});' command,
*               sequoiadb process(data node) while core dump.
*@Modify list :
*               2015-2-10  xiaojun Hu  Init
                2020-08-20 Zixian Yan  Modify
*******************************************************************************/
testConf.clName = COMMCLNAME + "_13755";
main( test );


function test ( testPara )
{
   var insertNum = 100;
   var cl = testPara.testCL;
   // insert data
   idxAutoGenData( cl, insertNum );

   // Test Point
   try
   {
      println(cl.find().sort( { "": "a" } ));
   }
   catch( error )
   {
      if( error != -6 )
      {
         throw new Error( "failed to test command: 'cl.find().sort( {\"\":\"a\"} )'. " + error );
      }
   }
}

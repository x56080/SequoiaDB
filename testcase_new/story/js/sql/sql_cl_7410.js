/****************************************************
@description:	create/drop CL by SQL, basic case
         testlink cases:   seqDB-7410/7412/7417
@input:        1 The cs does not exist, create cl in the cs. errorno: -34
               2 create cl, success
               3 exec [list collections], success
               4 create the same cl again, errorno: -33
               5 drop cl, success
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
testConf.csName = COMMCSNAME, testConf.csOpt = { PageSize: 4096 };
testConf.clName = CHANGEDPREFIX + "_7410", testConf.clOpt = {};

main( test );

function test ()
{
   var tmpCS = CHANGEDPREFIX + "_foo";

   try
   {
      db.execUpdate( "create collection " + tmpCS + "." + testConf.clName );
   }
   catch( e )
   {
      if( e.message != -34 )
      {
         throw new Error( "Failed to create CL in the CS when the CS does not exist. Expected errorno: -34" );
      }
   }

   db.getCS( testConf.csName ).getCL( testConf.clName );

   var rc = db.exec( "list collections" );
   if( 0 === rc.size() )
   {
      throw "Failed to compare results.";
   }

   try
   {
      db.execUpdate( "create collection " + testConf.csName + "." + testConf.clName );
   }
   catch( e )
   {
      if( e.message != -22 )
      {
         throw new Error( "create the same CL again,success. Expected errorno: -22" );
      }
   }
}
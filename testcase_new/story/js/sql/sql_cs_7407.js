/****************************************************
@description:	create/drop CS by SQL, basic case
         testlink cases:   seqDB-7407/7409/7416
@input:        1 create cs, success
               2 exec [list collectionspases], success
               3 create the same cs again, errorno: -33
               4 drop cs, success
               5 create cs again, success
@modify list:
      2015-5-13 ShanShan Hu added  2016-3-16 XiaoNi Huang modify
****************************************************/
testConf.csName = CHANGEDPREFIX + "_7407", testConf.csOpt = { PageSize: 4096 };

main( test );

function test ()
{

   var rc = db.getCS( testConf.csName );

   var rc = db.exec( "list collectionspaces" );
   if( 0 === rc.size() )
   {
      throw new Error( "Failed to compare results." );
   }
   try
   {
      db.execUpdate( "create collectionspace " + testConf.csName );
   }
   catch( e )
   {
      if( e.message != -33 )
      {
         throw new Error( e );
      }
   }

   db.execUpdate( "drop collectionspace " + testConf.csName );

   db.execUpdate( "create collectionspace " + testConf.csName );

   var rc = db.getCS( testConf.csName );
}
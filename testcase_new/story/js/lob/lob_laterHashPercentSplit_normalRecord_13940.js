/******************************************************************************
*@Description : the collection do hash percent split after input data.test
*               input normal record and lob data into collection
*@Modify list :
*               2014-12-18  xiaojun Hu  Init
******************************************************************************/

function main ( db )
{
   var testFile = CHANGEDPREFIX + "lobTest.file";
   var getTestFile = CHANGEDPREFIX + "lobTestGet.file";
   var putNum = 50;
   var partitionNum = 2048;

   var names = lobGetAllGroupNames( db );
   if( 1 == names.length )
   {
      return;
   }

   lobGenerateFile( testFile ); // auto file
   var originMd5 = getMd5ForFile( testFile );
   // create collection
   var optionObj = {
      "ShardingKey": { "no": 1 }, "ShardingType": "hash", "ReplSize": 0,
      "Partition": partitionNum, "Compressed": true
   };
   var cl = commCreateCLByOption( db, COMMCSNAME, COMMCLNAME, optionObj, true,
      true, "create collection for hash split" );
   // put normal data and lob data
   try
   {
      lobInsertDoc( cl, putNum );
      println( "success to put lob data" );
      var oids = lobPutLob( cl, testFile, putNum );
      println( "success to put normal record data" );
      var FULLCLNAME = COMMCSNAME + "." + COMMCLNAME;
      var clRg = commGetCLGroups( db, FULLCLNAME );
      var cond = Math.floor( 100 / names.length );
      for( var i = 0; i < names.length; ++i )
      {
         if( clRg[0] != names[i] )
         {
            var firstCond = cond;
            lobSplit( cl, clRg[0], names[i], firstCond, "percent" );
            firstCond += cond;
         }
      }
      println( "success to do hash percent split after input data" );
      for( var i = 0; i < cl.count(); ++i )
      {
         var count = cl.find( { "no": i } ).count();
         if( 1 != count )
         {
            println( "failed to query data, rc = " + cl.find( { "no": i } ) );
            throw "ErrNumberQuery";
         }
      }
      println( "success to query records" );
      for( var i = 0; i < oids.length; ++i )// will split error
      {
         cl.getLob( oids[i], getTestFile, true );
         var curMd5 = getMd5ForFile( getTestFile );
         if( originMd5 !== curMd5 )
         {
            throw "origin file's md5=" + originMd5 + "getLob's md5=" + curMd5;
         }
      }
   }
   catch( e )
   {
      println( "failed to normal record and lob data in collection, rc = " + e );
      throw e;
   }
   finally
   {
      var cmd = new Cmd();
      cmd.run( "rm -rf " + testFile );
      if( lobFileIsExist( getTestFile ) )
      {
         cmd.run( "rm -rf " + getTestFile );
      }
   }
}

// Run Main
try
{
   if( !commIsStandalone( db ) )
   {
      commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
         "clear collection in the beginning" );
      main( db );
   }
}
catch( e )
{
   throw e;
}
finally
{
   commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
      "drop collection in the end, error" );
   db.close();
}

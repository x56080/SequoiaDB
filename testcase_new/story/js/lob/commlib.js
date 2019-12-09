/******************************************************************************
*@Description : Test the hint index common function.
*@Modify list :
*               2014-6-12   xiaojun Hu  Init
*               2014-11-10  xiaojun Hu  Change
******************************************************************************/
function lobFileIsExist ( fileName )
{
   var isExist = false;
   try
   {
      var cmd = new Cmd();
      cmd.run( "ls " + fileName );
      isExist = true;
   }
   catch( e )
   {
      if( 2 == e ) { isExist = false; }
   }
   return isExist;
}

function getMd5ForFile ( testFile )
{
   try
   {
      var cmd = new Cmd();
      var md5Arr = cmd.run( "md5sum " + testFile ).split( " " );
      var md5 = md5Arr[0];
   }
   catch( e )
   {
      throw e;
   }
   return md5;
}

function lobGenerateFile ( fileName, fileLine )
{
   if( undefined == fileLine )
   {
      fileLine = 1000;
   }

   try
   {
      var cnt = 0;
      while( true == lobFileIsExist( fileName ) )
      {
         File.remove( fileName );
         if( cnt > 10 ) break;
         cnt++;
         sleep( 10 );
      }

      if( 10 <= cnt )
         throw "failed to remove file: " + fileName;
      var file = new File( fileName );
      for( var i = 0; i < fileLine; ++i )
      {
         var record = '{ no:' + i + ', score:' + i + ', interest:["movie", "photo"], ' +
            '  major:"计算机软件与理论", dep:"计算机学院", ' +
            '  info:{name:"Holiday", age:22, sex:"男"} }';
         file.write( record );
      }
      if( false == lobFileIsExist( fileName ) )
         throw "NoFile: " + fileName;
   }
   catch( e )
   {
      println( "faile to auto generate file, rc = " + e );
      throw e;
   }
}

function lobCreateCS ( db, DOMCSNAME, domName )
{
   try
   {
      var cs = db.createCS( DOMCSNAME, { "PageSize": 4096, "Domain": domName } );
      return cs;
   }
   catch( e )
   {
      println( "failed to create collection space attach domain, rc = " + e );
      throw e;
   }
}

function lobPutLob ( cl, lobFile, lobNum )
{
   if( undefined == lobNum )
   {
      lobNum = 10;
   }

   var oid = [];
   try
   {
      for( var i = 0; i < lobNum; ++i )
      {
         oid[i] = cl.putLob( lobFile );
      }
      // verify
      var cursor = cl.listLobs().toArray();
      if( lobNum != cursor.length )
      {
         println( "collection have lob: " + cursor.length );
         throw "ErrNumberPutLob";
      }
      return oid;
   }
   catch( e )
   {
      println( "failed to put lob in collection, rc = " + e );
      throw e;
   }
}

// Insert data to SequoiaDB
function lobInsertDoc ( cl, recordNum )
{
   try
   {
      var docs = [];
      // insert 10000 records in CL
      for( var i = 0; i < recordNum; ++i )
      {
         docs.push(
            {
               no: i, score: i, interest: ["movie", "photo"],
               major: "计算机软件与理论", dep: "计算机学院",
               info: { name: "Holiday", age: 22, sex: "男" }
            } );

      }
      cl.insert( docs );
      var cnt = 0;
      do
      {
         ++cnt;
         sleep( 10 );
      }
      while( recordNum != cl.count() && 1000 < cnt );
      if( recordNum != cl.count() )
      {
         println( "wrong quantity of records, count = " + count );
         throw "ErrNumRecords";
      }
   }
   catch( e )
   {
      println( "failed to insert data to collection, rc = " + e );
      throw e;
   }
}

function lobSplit ( cl, srcGroup, dstGroup, firstCond, secondCond )
{
   try
   {
      if( undefined == srcGroup ) { throw "NoSourceGroup"; }
      if( undefined == dstGroup ) { throw "NoDestnationGroup"; }
      if( undefined == secondCond ) { secondCond = "percent"; }
      if( "percent" != secondCond )
      {
         println( "first condition: " + JSON.stringify( firstCond ) );
         println( "second condition: " + JSON.stringify( secondCond ) );
         cl.split( srcGroup, dstGroup, firstCond, secondCond );
      }
      else
      {
         println( "percent condition: " + JSON.parse( firstCond ) );
         cl.split( srcGroup, dstGroup, firstCond );
      }
   }
   catch( e )
   {
      println( "failed to split collection, rc = " + e );
      throw e;
   }
}

function lobGetAllGroupNames ( db )
{
   try
   {
      var RG = commGetGroups( db );
      var groupnames = [];
      for( var i = 0; i < RG.length; ++i )
      {
         groupnames.push( RG[i][0].GroupName );
      }
      return groupnames;
   }
   catch( e )
   {
      println( "failed to get groups, rc = " + e );
      throw e;
   }
}

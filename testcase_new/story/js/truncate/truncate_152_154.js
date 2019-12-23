/******************************************************************************
*@Description: testcases for normal table
*@Modify list:
*              2015-5-8  xiaojun Hu   Init
******************************************************************************/

try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ()
{
   var clName = COMMCLNAME + "_152";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop collection begin" );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false,
      "create collection begin" );
   var tableName = COMMCSNAME + "." + clName;

   testTruncateNormalTblRecord( db, cl, tableName );

   testTruncateNormalTblLob( db, cl, tableName );

   testTruncateNormalTable( db, cl, tableName );

   commDropCL( db, COMMCSNAME, clName, false, false, "drop collection end" );
}

/*******************************************************************************
*@Description: 测试truncate()对普通表内的普通记录数据的操作
*@Input: collection.truncate()
*@Expectation: 清除数据及数据页成功
********************************************************************************/
function testTruncateNormalTblRecord ( db, cl, tableName )
{
   var funcName = "testTruncateNormalTblRecord";
   var verfify = { "TotalDataPages": 1 };
   truncateVerify( db, tableName );

   // insert 13 types
   truncateInsertRecord( cl );
   truncateVerify( db, tableName, verfify );

   // truncate
   cl.truncate();
   if( 0 != cl.count() )
   {
      throw new Error( "cl.count(): " + cl.count() );
   }

   truncateVerify( db, tableName );
}

/*******************************************************************************
*@Description: 测试truncate()对普通表内的大对象[LOB]数据的操作
*@Input: collection.truncate()
*@Expectation: 清除大对象[LOB]数据及LOB数据页成功
********************************************************************************/
function testTruncateNormalTblLob ( db, cl, tableName )
{
   var funcName = "testTruncateNormalTblLob";
   var verfify = { "TotalLobPages": 5 };
   if( undefined == db ) { throw new Error( "db: " + db ); }
   if( undefined == cl ) { throw new Error( "cl: " + cl ); }
   if( undefined == tableName ) { throw new Error( "tableName: " + tableName ); }
   var lobSize = 2049;
   var lobNumber = 5;
   truncateVerify( db, tableName );

   // pub Lob
   truncatePutLob( cl, lobSize, lobNumber );
   truncateVerify( db, tableName, verfify );

   // truncate
   cl.truncate();
   listLobs = cl.listLobs().toArray();
   if( 0 != listLobs.length )
   {
      throw new Error( "listLobs.length: " + listLobs.length );
   }

   truncateVerify( db, tableName );
}

/*******************************************************************************
*@Description: 测试truncate()对普通表内的普通记录和大对象[LOB]数据的操作
*@Input: collection.truncate()
*@Expectation: 清除普通记录和大对象[LOB]数据成功, 清除数据页和LOB数据页成功
********************************************************************************/
function testTruncateNormalTable ( db, cl, tableName )
{
   var funcName = "testTruncateNormalTable";
   var verfify = { "TotalDataPages": 1, "TotalLobPages": 5 };
   if( undefined == db ) { throw new Error( "db: " + db ); }
   if( undefined == cl ) { throw new Error( "cl: " + cl ); }
   if( undefined == tableName ) { throw new Error( "tableName: " + tableName ); }
   var lobSize = 2049;
   var lobNumber = 5;
   truncateVerify( db, tableName );

   // pub Lob and insert record, record have cursor for lob
   var cursor = truncatePutLob( cl, lobSize, lobNumber );
   for( var i = 0; i < cursor.length; ++i )
   {
      truncateInsertRecord( cl, 1, lobSize, { "LobOID": cursor[i] } );
   }
   // truncate
   cl.truncate();
   if( 0 != cl.count() )
   {
      throw new Error( "cl.count(): " + cl.count() );
   }
   listLobs = cl.listLobs().toArray();
   if( 0 != listLobs.length )
   {
      throw new Error( "listLobs.length: " + listLobs.length );
   }

   truncateVerify( db, tableName );
}

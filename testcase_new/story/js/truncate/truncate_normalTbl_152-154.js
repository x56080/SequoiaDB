/******************************************************************************
*@Description: testcases for normal table
*@Modify list:
*              2015-5-8  xiaojun Hu   Init
******************************************************************************/

main();
function main ()
{
   var clName = "truncate152";
   commDropCL( db, COMMCSNAME, clName, true, true, "drop collection begin" );
   var cl = commCreateCL( db, COMMCSNAME, clName, 0, true, true, false,
      "create collection begin" );
   var tableName = COMMCSNAME + "." + clName;

   println( "\n---begin to test <testTruncateNormalTblRecord>" );
   testTruncateNormalTblRecord( db, cl, tableName );

   println( "\n---begin to test <testTruncateNormalTblLob>" );
   testTruncateNormalTblLob( db, cl, tableName );

   println( "\n---begin to test <testTruncateNormalTable>" );
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
   try
   {
      var verfify = { "TotalDataPages": 1 };
      truncateVerify( db, tableName );

      // insert 13 types
      truncateInsertRecord( cl );
      truncateVerify( db, tableName, verfify );
      // truncate
      cl.truncate();
      if( 0 != cl.count() )
      {
         println( "expect insert: 0, actual insert: " + cl.count() );
         throw "error insert number";
      }

      truncateVerify( db, tableName );
   }
   catch( e )
   {
      throw buildException( funcName, e );
   }
}

/*******************************************************************************
*@Description: 测试truncate()对普通表内的大对象[LOB]数据的操作
*@Input: collection.truncate()
*@Expectation: 清除大对象[LOB]数据及LOB数据页成功
********************************************************************************/
function testTruncateNormalTblLob ( db, cl, tableName )
{
   var funcName = "testTruncateNormalTblLob";
   try
   {
      var verfify = { "TotalLobPages": 5 };
      if( undefined == db ) { throw "no sdb connect handle"; }
      if( undefined == cl ) { throw "no collection handle"; }
      if( undefined == tableName ) { throw "no pub table name"; }
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
         println( listLobs );
         println( "expect lob number: 0, actual: " + listLobs.length );
         throw "error lob numbers after truncate";
      }

      truncateVerify( db, tableName );
   }
   catch( e )
   {
      throw buildException( funcName, e );
   }
}

/*******************************************************************************
*@Description: 测试truncate()对普通表内的普通记录和大对象[LOB]数据的操作
*@Input: collection.truncate()
*@Expectation: 清除普通记录和大对象[LOB]数据成功, 清除数据页和LOB数据页成功
********************************************************************************/
function testTruncateNormalTable ( db, cl, tableName )
{
   var funcName = "testTruncateNormalTable";
   try
   {
      var verfify = { "TotalDataPages": 1, "TotalLobPages": 5 };
      if( undefined == db ) { throw "no sdb connect handle"; }
      if( undefined == cl ) { throw "no collection handle"; }
      if( undefined == tableName ) { throw "no pub table name"; }
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
         println( "expect record number: 0, actual record number: " + cl.count() );
         throw "error record numbers after truncate";
      }
      listLobs = cl.listLobs().toArray();
      if( 0 != listLobs.length )
      {
         println( listLobs );
         println( "expect lob number: 0, actual: " + listLobs.length );
         throw "error lob numbers after truncate";
      }

      truncateVerify( db, tableName );
   }
   catch( e )
   {
      throw buildException( funcName, e );
   }
}




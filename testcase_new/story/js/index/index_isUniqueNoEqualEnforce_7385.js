/******************************************************************************
*@Description: create Index when use:{isUnique:false, enforced: true}
*              [ JIRA: SEQUOIADBMAINSTREAM-840 ]
*@Modify list:
*              2014-5-5  xiaojun Hu   Init
               2016-2-27 yan wu  modify：删除清理环境中的dropIndex
******************************************************************************/

/*******************************************************************************
*@Description: 创建的索引是强制唯一时, isUnique为false, enforced为true
*@Input: cl.createIndex( indexName, {a:1}, false, true )
*@Expectation: 创建索引失败，报-6错误【Invalid Argument】
********************************************************************************/
function testIsUniqueFalse ( cl, indexName )
{
   try
   {
      if( undefined == cl ) { throw "no collection handle"; }
      if( undefined == indexName ) { throw "no index name"; }
      var funcName = "testIsUniquefalse";
      // verify ("indexName", {a:1}, false, true)
      try
      {
         cl.createIndex( indexName, { a: 1 }, false, true );
         throw "create index should throw error";
      }
      catch( e )
      {
         if( -6 != e )
         {
            println( "create index error, rc = " + e );
            throw e;
         }

      }

   }
   catch( e )
   {
      throw buildException( funcName, e );
   }
}

/*******************************************************************************
*@Description: 创建的索引是强制唯一时, isUnique和enforced均为true
*@Input: cl.createIndex( indexName, {a:1}, false, true )
*@Expectation: 创建索引成功. 且表中不可存在相同的记录
********************************************************************************/
function testIsUniqueEnforced ( cl, indexName )
{
   try
   {
      if( undefined == cl ) { throw "no collection handle"; }
      if( undefined == indexName ) { throw "no index name"; }
      var funcName = "testIsUniqueEnforced";
      cl.createIndex( indexName, { a: 1 }, true, true );
      var query = cl.find( { a: { $lt: 10 } } ).hint( { '': '' } ).explain( { Run: true } ).toArray();
      var queryObj = eval( "(" + query[0] + ")" );
      if( "ixscan" != queryObj["ScanType"] || 2 != queryObj["ReturnNum"] )
      {
         println( "explain query: " + query );
         throw "wrong query";
      }
      // verify ("indexName", {a:1}, true, true)
      try
      {
         cl.insert( { a: -5 } );
         throw "should not insert data success, need throw error";
      }
      catch( e )
      {
         if( -38 != e )
         {
            println( "insert error, rc = " + e );
            throw e;
         }

      }
   }
   catch( e )
   {
      throw buildException( funcName, e );
   }
}

function main ()
{
   try
   {
      var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      var indexName1 = CHANGEDPREFIX + "_indexName1";
      var indexName2 = CHANGEDPREFIX + "_indexName2";
      commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
         "drop collection begin" );
      var cl = commCreateCL( db, COMMCSNAME, COMMCLNAME, 0, true, true, false,
         "failed create collection in the beginning" );
      cl.insert( { a: 3 } );
      cl.insert( { a: -5 } );
      commDropIndex( cl, indexName1, true );
      commDropIndex( cl, indexName2, true );
      testIsUniqueFalse( cl, indexName1 );
      testIsUniqueEnforced( cl, indexName2 )
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      commDropCL( db, COMMCSNAME, COMMCLNAME, true, true,
         "drop collection end " );
   }
}

main();

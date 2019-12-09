/******************************************************************************
*@Description: create Index failed when use $ symbol in indexDef key
*              [ JIRA: SEQUOIADBMAINSTREAM-820 ]
*@Modify list:
*              2014-5-5  xiaojun Hu   Init
******************************************************************************/

/*******************************************************************************
*@Description: 在普通表上创建索引时, 指定Key的名字有"$"符号
*@Input: cl.createIndex( "indexName", { "a.$0.b": 1 })
*@Expectation: 创建索引失败，报-6错误【Invalid Argument】
********************************************************************************/
function testNormalClIndex ( db )
{
   try
   {
      var funcName = "testNormalClIndex";
      var clName = CHANGEDPREFIX + "_idxNormalCl";
      var indexName = CHANGEDPREFIX + "_index";
      var indexDef1 = { "a.$0.b": 1 };
      var indexDef2 = { "a$b": 1 };
      var indexDef3 = { "f.d": -1, "a.$0.b": 1 };
      commDropCL( db, COMMCSNAME, clName, true, true,
         "drop collection begin, " + funcName );
      var cl = commCreateCL( db, COMMCSNAME, clName, 0, true, true, false,
         "failed create collection in the beginning" );
      cl.insert( { "a": [{ "b": 1 }, { "c": 2 }] } );

      // { "a.$0.b": 1 }
      commDropIndex( cl, indexName, true );
      try
      {
         cl.createIndex( indexName, indexDef1 );
         throw "need throw error";
      }
      catch( e )
      {
         if( -6 != e )
         {
            println( "failed to test create index:{ \"a.$0.b\": 1 }, rc = " + e );
            throw e;
         }
      }

      // { "a$b": 1 }
      commDropIndex( cl, indexName, true );
      try
      {
         cl.createIndex( indexName, indexDef2 );
         throw "need throw error";
      }
      catch( e )
      {
         if( -6 != e )
         {
            println( "failed to test create index:{ \"a$b\": 1 }, rc = " + e );
            throw e;
         }
      }

      // { "f.d":-1, "a.$0.b": 1 }
      commDropIndex( cl, indexName, true );
      try
      {
         cl.createIndex( indexName, indexDef3 );
         throw "need throw error";
      }
      catch( e )
      {
         if( -6 != e )
         {
            println( "failed to test create index:" +
               "{\"f.d\":-1,\"a.$0.b\":1}, rc = " + e );
            throw e;
         }
      }
   }
   catch( e )
   {
      throw buildException( funcName, e );
   }
   finally
   {
      commDropCL( db, COMMCSNAME, clName, false, false,
         "drop collection end, " + funcName );
   }
}

/*******************************************************************************
*@Description: 在主子表上创建索引时, 指定Key的名字有"$"符号
*@Input: cl.createIndex( "indexName", { "a.$0.b": 1 })
*@Expectation: 创建索引失败，报-6错误【Invalid Argument】
********************************************************************************/
function testMainSubClIndex ( db )
{
   try
   {
      var funcName = "testMainSubClIndex";
      var mainClName = CHANGEDPREFIX + "_indexMainCl";
      var subClName = CHANGEDPREFIX + "_indexSubCl";
      var indexName = CHANGEDPREFIX + "_indexMainSubCl";
      var indexDef = { "a.$0.b": 1 };
      var optionObj = { "ShardingKey": { a: 1 }, "IsMainCL": true };
      commDropCL( db, COMMCSNAME, mainClName, true, true,
         "drop main collection begin, " + funcName );
      commDropCL( db, COMMCSNAME, subClName, true, true,
         "drop sub collection begin, " + funcName );
      var cl = commCreateCLByOption( db, COMMCSNAME, mainClName, optionObj, true,
         false,
         "failed create collection in the beginning" );
      commCreateCL( db, COMMCSNAME, subClName, 0, true, true, false,
         "failed create collection in the beginning" );
      commDropIndex( cl, indexName, true );
      try
      {
         cl.attachCL( COMMCSNAME + "." + subClName, { LowBound: { a: 1 }, UpBound: { a: 10 } } );
         println( "success to attach cl" );
         cl.insert( { a: 1 } );
         cl.insert( { a: 9 } );
         cl.createIndex( indexName, indexDef, false, true );
         throw "need throw error";
      }
      catch( e )
      {
         if( -6 != e )
         {
            println( "failed to test create index, rc = " + e );
            throw e;
         }
      }
   }
   catch( e )
   {
      throw buildException( funcName, e );
   }
   finally
   {
      commDropCL( db, COMMCSNAME, mainClName, false, false,
         "drop main collection end, " + funcName );
      /*   drop main collection will drop sub collection when in same CS
      commDropCL( db, COMMCSNAME, subClName, false, false,
                  "drop sub collection end, " + funcName );
      */
   }
}

/*******************************************************************************
*@Description: 在水平分区表上创建索引时, 指定Key的名字有"$"符号
*@Input: cl.createIndex( "indexName", { "a.$0.b": 1 })
*@Expectation: 创建索引失败，报-6错误【Invalid Argument】
********************************************************************************/
function testSplitClIndex ( db )
{
   try
   {
      var funcName = "testSplitClIndex";
      var clName = CHANGEDPREFIX + "_idxSplitCl";
      var indexName = CHANGEDPREFIX + "_indexSplitCl";
      var domainName = CHANGEDPREFIX + "_domainIdxSplit";
      var indexDef = { "a.$0.b": 1 };
      var optionObj = {
         "ShardingKey": { a: -1 }, "ShardingType": "hash",
         "AutoSplit": true
      };
      commDropCL( db, COMMCSNAME, clName, true, true,
         "drop main collection begin, " + funcName );
      var cl = commCreateCLByOption( db, COMMCSNAME, clName, optionObj, true, false,
         "failed create collection in the beginning" );
      commDropIndex( cl, indexName, true );
      commDropDomain( db, domainName );
      try
      {
         var groups = commGetGroups( db );
         var domainGroups = new Array();
         for( var i = 0; i < groups.length; ++i )
         {
            domainGroups[i] = groups[i][0].GroupName;
         }
         println( "group: " + domainGroups );
         commCreateDomain( db, domainName, domainGroups, { "AutoSplit": true } );
      }
      catch( e )
      {
         println( "failed to create domain, rc = " + e );
         throw e;
      }

      try
      {
         cl.insert( { a: 9 } );
         cl.createIndex( indexName, indexDef, false, true );
         throw "need throw error";
      }
      catch( e )
      {
         if( -6 != e )
         {
            println( "failed to test create index:, rc = " + e );
            throw e;
         }
      }
   }
   catch( e )
   {
      throw buildException( funcName, e );
   }
   finally
   {
      commDropCL( db, COMMCSNAME, clName, false, false,
         "drop main collection end, " + funcName );
      commDropDomain( db, domainName );
   }
}

function main ()
{
   try
   {
      try
      {
         var db = new Sdb( COORDHOSTNAME, COORDSVCNAME );
      }
      catch( e )
      {
         println( "failed to connect to db, rc = " + e );
         throw e;
      }
      // normal cl
      testNormalClIndex( db );
      if( false == commIsStandalone( db ) )
      {
         // main and sub cl
         testMainSubClIndex( db );
         // split cl
         testSplitClIndex( db );
      }
   }
   catch( e )
   {
      throw e;
   }
}

main();

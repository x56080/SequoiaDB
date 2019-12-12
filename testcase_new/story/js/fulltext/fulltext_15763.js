/***************************************************************************
@Description :seqDB-15763 :更新使记录的_id索引重复 
@Modify list :
              2018-10-09  YinZhen  Create
****************************************************************************/
function main ()
{

   if( commIsStandalone( db ) )
   {
      println( "Deploy is standalone" );
      return;
   };

   var clName = COMMCLNAME + "_ES_15763";
   commDropCL( db, COMMCSNAME, clName, true, true );

   //创建集合并创建全文索引
   var dbcl = commCreateCL( db, COMMCSNAME, clName, 0 );
   var textIndexName = "textIndexName_ES_15763";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );

   //插入包含_id索引和全文索引的记录
   var records = new Array();
   records[0] = { _id: 1001, about: "about for you", content: "this is my college" };
   records[1] = { _id: 1002, about: "how it go on", content: "this is my hometown" };
   dbcl.insert( records );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 2 );

   var esOperator = new ESOperator();
   var queryCond = '{"query" : {"exists" : {"field" : "content"}}}';
   var findConf = { "": { $Text: { "query": { "exists": { "field": "content" } } } } };
   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var actCLRecords = dbOperator.findFromCL( dbcl, findConf, null, null, null );

   var expESRecords = new Array();
   expESRecords.push( { about: "about for you", content: "this is my college" } );
   expESRecords.push( { about: "how it go on", content: "this is my hometown" } );
   var expCLRecords = records;

   //记录插入成功，原始集合、固定集合及ES端记录正确
   checkRecords( expESRecords, actESRecords );
   checkRecords( expCLRecords, actCLRecords );

   //更新_id字段的值与集合中其它任意一条记录的_id索引重复，检查结果
   updateFieldId( dbcl );

   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var actCLRecords = dbOperator.findFromCL( dbcl, findConf, null, null, null );

   //报错-38，检查原始集合、固定集合及ES记录的_id索引未被更新，使用inspect工具查看主备数据节点数据无差别
   checkRecords( expESRecords, actESRecords );
   checkRecords( expCLRecords, actCLRecords );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   commDropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function updateFieldId ( dbcl )
{
   try
   {
      dbcl.update( { $set: { _id: 1002 } }, { _id: 1001 } );
      throw new Error( "update error!" );
   }
   catch( e )
   {
      if( e.message != -38 )
      {
         throw e;
      }
   }
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "about" ) );
   actRecords.sort( compare( "abour" ) );
   checkResult( expRecords, actRecords )
}

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
;

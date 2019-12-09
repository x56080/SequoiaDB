/******************************************************************************
@Description : 1. sub-CL sort
@Modify list :
               2015-01-16 pusheng Ding  Init
******************************************************************************/
main();
function main () 
{
   if( commIsStandalone( db ) )
   {
      println( " Deploy mode is standalone!" );
      return;
   }

   var csName = COMMCSNAME;
   var subCLName1 = "subcl13744_1";
   var subCLName2 = "subcl13744_2";
   var subCLName3 = "subcl13744_3";
   var mainclName = "maincl13744";
   var indexName = "index13744";
   var rownums = 10000;

   commDropCL( db, csName, subCLName1, true, true, "drop sub cl1 in the beginning" );
   commDropCL( db, csName, subCLName2, true, true, "drop sub cl2 in the beginning" );
   commDropCL( db, csName, subCLName3, true, true, "drop sub cl3 in the beginning" );
   commDropCL( db, csName, mainclName, true, true, "drop main cl in the beginning" );

   var options = { ShardingKey: { a: 1 }, ReplSize: 0, IsMainCL: true };
   var mainCL = commCreateCLByOption( db, csName, mainclName, options, true, false, "create main cl." );

   var suboptions1 = { ShardingKey: { b: 1 }, ShardingType: "hash", Partition: 4096 };
   commCreateCLByOption( db, csName, subCLName1, suboptions1, true, false, "create sub cl 1." );

   var suboptions2 = { ReplSize: 2 };
   commCreateCLByOption( db, csName, subCLName2, suboptions2, true, false, "create sub cl 2." );

   var suboptions3 = { Compressed: true };
   commCreateCLByOption( db, csName, subCLName3, suboptions3, true, false, "create sub cl 3." );

   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { a: -10000 }, UpBound: { a: 0 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { a: 0 }, UpBound: { a: 6000 } } );
   mainCL.attachCL( csName + "." + subCLName3, { LowBound: { a: 6000 }, UpBound: { a: 20000 } } );

   //insert data
   var records = readyData( mainCL, rownums );

   //query1 切分键索引排序
   //select a,b,c from foo.bar order by a desc
   var sel = mainCL.find().sort( { a: -1 } );
   checkSortResult( mainCL, records, -1, sel );
   println( "'select a,b,c from foo.bar order by a desc' finished!" );

   //create index
   mainCL.createIndex( indexName, { a: 1, b: 1 }, true );

   //query2 普通索引排序
   //select b from foo.bar order by b
   db.setSessionAttr( { PreferedInstance: 'M' } );
   // 走索引查询
   var selExplain = mainCL.find( null, { b: 0 } ).sort( { b: 1 } ).hint( { "": indexName } ).explain().toArray();
   for( var j = 0; j < selExplain.length; ++j )
   {
      var selObj = eval( "(" + selExplain[j] + ")" );
      if( "ixscan" != selObj["SubCollections"][0]["ScanType"] )
      {
         println( "explain: " + selExplain[j] );
         throw "failed to run index query";
      }
   }
   var sel = mainCL.find().sort( { b: 1 } ).hint( { "": indexName } );
   //expected result {b:0} {b:1} ... {b:rownums-1}
   checkSortResult( mainCL, records, 1, sel );
   println( "'select a,b,c from foo.bar order by b' finished!" );

   mainCL.detachCL( COMMCSNAME + "." + subCLName1 );
   mainCL.detachCL( COMMCSNAME + "." + subCLName2 );
   mainCL.detachCL( COMMCSNAME + "." + subCLName3 );
   println( "detach sub-CL finish!" );

   commDropCL( db, csName, subCLName1, false, false, "drop sub cl1 cl in the end" );
   commDropCL( db, csName, subCLName2, false, false, "drop sub cl2 in the end" );
   commDropCL( db, csName, subCLName3, false, false, "drop sub cl3 in the end" );
   commDropCL( db, csName, mainclName, false, false, "drop main cl in the end" );
}

function readyData ( cl, insertNum )
{
   try
   {
      var orderedRecords = new Array();
      println( "\n---Begin to insert cl data." );
      var records = new Array();
      for( var i = 0; i < insertNum; i++ )
      {
         records[i] = { _id: i, a: i, b: i, c: "strTest13744_" + i };
      }
      //将有序的数组中数据保存在orderedRecords并返回（使用slice()避免将原数组的引用赋值给orderedRecords）
      var orderedRecords = records.slice();
      //将有序的数组中数据打乱顺序并插入到集合中
      var randomRecords = records.sort( function() { return 0.5 - Math.random() } );
      cl.insert( randomRecords );
      return orderedRecords;
   }
   catch( e )
   {
      throw buildException( "readyData()", e, "insert", "insert success", "insert fail" );
   }
}

function checkSortResult ( cl, records, sortOrder, sel )
{
   var expRecords = records.slice();
   if( sortOrder == -1 )
   {
      expRecords.reverse();
   }
   checkRec( sel, expRecords );
}

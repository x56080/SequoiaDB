/************************************
*@Description：清理range表切分任务时创建同名hash分区表 
*@author：2019-7-20 wuyan
*@testlinkCase: seqDB-18894
**************************************/
main();
function main ()
{
   var csName = "splitcs_18894";
   var clName = "splitcl_18894";

   if( true == commIsStandalone( db ) )
   {
      println( "run mode is standalone" );
      return;
   }

   var allGroupName = getGroupName2( db, true );
   if( 2 > allGroupName.length )
   {
      println( "--least two groups" );
      return;
   }

   commDropCS( db, csName, true, true, "drop CS in the beginning." );;

   var groupsInfo = getGroupName2( db, true );
   var srcGrName = groupsInfo[0][0];
   var tarGrName = groupsInfo[1][0];

   var options = { ShardingKey: { a: 1 }, ShardingType: "range", Group: srcGrName };
   var dbcs = db.createCS( csName );
   var dbcl = commCreateCL( db, csName, clName, options, false );
   var insertNum = 40000;
   insertData( dbcl, insertNum );
   dbcl.splitAsync( srcGrName, tarGrName, { a: 1 }, { a: 40000 } );

   println( "---begin to drop cl and create new cl." )
   dbcs.dropCL( clName );
   var dbcl1 = dbcs.createCL( clName, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: tarGrName } );
   dbcl1.insert( { a: 1 } );

   commDropCS( db, csName, true, true, "drop CS in the end." );
}

function insertData ( dbcl, insertNums )
{
   println( "---begin to insert datas." );
   var batchNums = 10000;
   var times = insertNums / batchNums;
   for( var k = 0; k < times; k++ )
   {
      var doc = [];
      for( var i = 0; i < batchNums; ++i )
      {
         var no = i;
         var a = i;
         var test = "test" + i;
         var objs = { "a": a, "no": no, "test": test };
         doc.push( objs );
      }
      dbcl.insert( doc );
   }
}

/************************************
*@Description:insert data over Max
*@author:      zhaoyu
*@createdate:  2017.7.15
*@testlinkCase: seqDB-12139
**************************************/
function main ()
{
   var csName = COMMCSNAME + "_12139";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   var csOption = { Capped: true };
   commCreateCS( db, csName, false, "", csOption );

   var clName = COMMCLNAME + "_12139";
   var insertNum = 10000;
   var clOption = { Capped: true, Size: 1024, Max: insertNum, AutoIndexId: false, OverWrite: true };
   var dbcl = commCreateCL( db, csName, clName, clOption, true, true );

   db.setSessionAttr( { PreferedInstance: "M" } );

   var expectNum = 0;
   var expectLogicalID = 0;
   var min = 0;
   var max = insertNum - 1;
   var repeatNum = 10;
   for( var j = 0; j < repeatNum; j++ )
   {
      //插入Max条记录
      var rd = new commDataGenerator();
      var recs = rd.getRecords( insertNum, ["int", "string", "bool", "date",
         "binary", "regex", "null"], ['a'] );
      insertDatas( dbcl, recs );
      println( "--insert data success! insertNum: " + insertNum );
      expectNum = expectNum + insertNum;

      //获取第1条及最后一条记录的_id值
      var firstLogicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, null );
      var lastLogicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, max );

      //获取数据占用块数
      var dataSize = lastLogicalID[0] - firstLogicalID[0];
      var blockNum = Math.ceil( ( dataSize ) / 33554395 );
      println( "--data occupy blockNums: " + blockNum + ",firstLogicalID: " + firstLogicalID + ",lastLogicalID: " + lastLogicalID );

      //再次插入1条记录并获取_id值
      var rd = new commDataGenerator();
      var recs = rd.getRecords( 1, ["int", "string", "bool", "date",
         "binary", "regex", "null"], ['a'] );
      insertDatas( dbcl, recs );
      println( "--insert data up to limit!" );
      lastLogicalID = getLogicalID( dbcl, null, null, { _id: -1 }, 1, null );

      //校验_id值是否正确
      var expectLogicalID = expectLogicalID + blockNum * 33554396;
      if( expectLogicalID !== lastLogicalID[0] )
      {
         println( "expectLogicalID: " + expectLogicalID + " ,actualID: " + lastLogicalID[0] );
         throw "LOGICAL_ID_ERROR";
      }
      println( "--check logical id success! logicalID: " + lastLogicalID[0] );

      //再次插入Max-1条记录
      var rd = new commDataGenerator();
      var recs = rd.getRecords( max, ["int", "string", "bool", "date",
         "binary", "regex", "null"], ['a'] );
      insertDatas( dbcl, recs );
      println( "--insert data success!" );

      //随机指定LogicalID
      var range = max - min;
      var skipNum = Math.ceil( min + Math.random() * range );
      var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, skipNum );

      //随机设置pop方向
      if( skipNum % 2 === 0 )
      {
         direction = -1;
      } else
      {
         direction = 1;
      }

      //执行pop
      pop( dbcl, logicalID[0], direction );

      //比较count结果
      if( direction == -1 )
      {
         expectNum = skipNum;
      } else
      {
         expectNum = expectNum - skipNum - 1;
      }
      checkCount( dbcl, null, expectNum );
      println( "--count success!expectCount: " + expectNum );

      //比较find结果
      try
      {
         dbcl.find().sort( { _id: 1 } ).limit( 1 );
         dbcl.find().sort( { _id: 1 } ).limit( 1 ).skip( expectNum - 1 );
      } catch( e )
      {
         throw buildException( "find data 1", e, null, null, e );
      }
      println( "--find data success!" );

      insertNum = max + 1 - expectNum;
      println( "repeat do " + j + " times success!" );
   }

   //校验主备数据一致
   checkConsistency( db, csName, clName );

   //最终主备数据一致
   db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "s" } );
   dbcl = db1.getCS( csName ).getCL( clName );

   //比较count结果
   actualNum = dbcl.count();
   if( parseInt( actualNum ) !== expectNum )
   {
      println( "--slave node count failed!actualNum:" + actualNum + ",expectNum: " + expectNum );
      throw "SECOND_COUNT_ERR";
   }

   //比较find结果
   try
   {
      dbcl.find().sort( { _id: 1 } ).limit( 1 );
      dbcl.find().sort( { _id: 1 } ).limit( 1 ).skip( expectNum - 1 );
   } catch( e )
   {
      throw buildException( "find data", e, null, null, e );
   }
   println( "--second node find data success!" );

   commDropCS( db, csName, true, "drop CS in the end" );
}
main();
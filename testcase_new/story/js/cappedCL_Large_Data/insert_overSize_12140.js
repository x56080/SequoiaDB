/************************************
*@Description:insert data over size
*@author:      zhaoyu
*@createdate:  2017.7.15
*@testlinkCase: seqDB-12140
**************************************/
function main ()
{
   var csName = COMMCSNAME + "_12140";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   var csOption = { Capped: true };
   commCreateCS( db, csName, false, "", csOption );

   var clName = COMMCLNAME + "_12140";
   var clOption = { Capped: true, Size: 32, AutoIndexId: false, OverWrite: true };
   var dbcl = commCreateCL( db, csName, clName, clOption, true, true );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );
   var db2 = new Sdb( db );
   db2.setSessionAttr( { PreferedInstance: "s" } );
   var dbclSlave = db2.getCS( csName ).getCL( clName );

   var insertNum = 32767;
   var firstBlockRecordNum = 0;
   var expectLogicalID = 0;
   var min = 0;
   var max = insertNum;
   var repeatNum = 10;
   var stringLength = 969;
   var recordHead = 55;
   var overturn = 0;
   var expIDs = [];
   var expectNum = 0;
   var expID = 0;
   for( var j = 0; j < repeatNum; j++ )
   {
      //插入刚好Size记录
      insertFixedLengthDatas( dbcl, max, stringLength, "a" );

      //计算预期的_id值
      expIDs = [];
      expID = 33554396 * overturn;
      for( var i = 0; i < max; i++ )
      {
         expIDs.push( expID );
         expID = expID + stringLength + recordHead;
      }

      //检查主备节点一致
      checkConsistency( db, csName, clName );

      //校验count是否正确
      expectNum = max;
      checkCount( dbclPrimary, null, expectNum );
      checkCount( dbclSlave, null, expectNum );

      //校验_id值
      checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
      checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

      //随机指定LogicalID
      var range = max - min;
      var skipNum = Math.ceil( min + Math.random() * range );
      println( "skipNum: " + skipNum );
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

      //检查主备节点一致
      checkConsistency( db, csName, clName );

      //根据方向不同校验结果并插入记录
      if( direction == -1 )
      {
         expectNum = skipNum;
         expIDs.splice( skipNum );
         expID = logicalID[0];

         //校验count是否正确
         checkCount( dbclPrimary, null, expectNum );
         checkCount( dbclSlave, null, expectNum );

         //校验_id值是否正确
         checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
         checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

         //再次插入填满本块的记录
         insertNum = max - expectNum;
         println( "insertNum: " + insertNum );
         insertFixedLengthDatas( dbcl, insertNum, stringLength, "a" );

         //计算预期的_id值
         for( var i = 0; i < insertNum; i++ )
         {
            expIDs.push( expID );
            expID = expID + stringLength + recordHead;
         }

         //检查主备节点一致
         checkConsistency( db, csName, clName );

         //校验count是否正确
         expectNum = max;
         checkCount( dbclPrimary, null, expectNum );
         checkCount( dbclSlave, null, expectNum );

         //校验_id值是否正确
         checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
         checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );
      } else
      {
         expectNum = expectNum - skipNum - 1;
         expIDs = expIDs.splice( skipNum + 1 );

         //校验count是否正确
         checkCount( dbclPrimary, null, expectNum );
         checkCount( dbclSlave, null, expectNum );

         //校验_id值是否正确
         checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
         checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );
      }
      println( "repeat do " + j + " times success!" );
      overturn++;
   }

   commDropCS( db, csName, true, "drop CS in the end" );
}
main();
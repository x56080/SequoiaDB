/************************************
*@Description:seqDB-11775:扩块/扩文件时插入记录/seqDB-11792:pop多个块的数据                 
*@author:      zhaoyu
*@createdate:  2017.7.17
*@testlinkCase: seqDB-11775/seqDB-11792
**************************************/
function main ()
{
   var csName = COMMCSNAME + "_11775";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   var csOption = { Capped: true };
   commCreateCS( db, csName, false, "", csOption );

   var clName = COMMCLNAME + "_11775";
   var clOption = { Capped: true, Size: 1024, AutoIndexId: false };
   var dbcl = commCreateCLByOption( db, csName, clName, clOption, true, true );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );
   var db2 = new Sdb( db );
   db2.setSessionAttr( { PreferedInstance: "s" } );
   var dbclSlave = db2.getCS( csName ).getCL( clName );

   //多次循环插入使扩文件
   var repeatNum = 30;
   for( var i = 0; i < repeatNum; i++ )
   {
      //插入1个块的记录
      insertNum = 32767;
      stringLength = 969;
      var recordHead = 55;
      var expectRecords = insertFixedLengthDatas( dbcl, insertNum, stringLength, "a" );
   }
   println( "--insert data success!" );

   //计算多个块内的预期的_id值
   var expIDs = [];
   var expID = 0;
   for( var i = 0; i < repeatNum; i++ )
   {
      for( var j = 0; j < insertNum; j++ )
      {
         expIDs.push( expID );
         expID = expID + stringLength + recordHead;
      }

      //跨块时，加上块尾的空隙
      var expID = expID + 988;
   }

   //检查主备节点一致
   checkConsistency( db, csName, clName );

   //校验多个块内的_id值
   checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
   checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

   //插入记录使扩展文件后，检查记录数
   var expectCount = repeatNum * insertNum;
   checkCount( dbcl, null, expectCount );
   println( "--check count success!" );

   //逆向pop单个块
   var skipNum = ( repeatNum - 1 ) * insertNum;
   var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, skipNum );
   pop( dbcl, logicalID[0], -1 );

   //计算多个块内的预期的_id值
   var expIDs = [];
   var expID = 0;
   for( var i = 0; i < repeatNum - 1; i++ )
   {
      for( var j = 0; j < insertNum; j++ )
      {
         expIDs.push( expID );
         expID = expID + stringLength + recordHead;
      }

      //跨块时，加上块尾的空隙
      var expID = expID + 988;
   }

   //检查主备节点一致
   checkConsistency( db, csName, clName );

   //校验多个块内的_id值
   checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
   checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

   //检查记录数
   expectCount = skipNum;
   checkCount( dbcl, null, expectCount );
   println( "--check count success!" );

   //逆向pop 2个块
   var skipNum = insertNum * ( repeatNum - 3 );
   var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, skipNum );
   pop( dbcl, logicalID[0], -1 );

   //计算多个块内的预期的_id值
   var expIDs = [];
   var expID = 0;
   for( var i = 0; i < repeatNum - 3; i++ )
   {
      for( var j = 0; j < insertNum; j++ )
      {
         expIDs.push( expID );
         expID = expID + stringLength + recordHead;
      }

      //跨块时，加上块尾的空隙
      var expID = expID + 988;
   }

   //检查主备节点一致
   checkConsistency( db, csName, clName );

   //校验多个块内的_id值
   checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
   checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

   //检查记录数
   expectCount = skipNum;
   checkCount( dbcl, null, expectCount );
   println( "--check count success!" );

   //正向pop单个块
   var skipNum = insertNum - 1;
   var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, skipNum );
   pop( dbcl, logicalID[0], 1 );

   //计算多个块内的预期的_id值
   var expIDs = [];
   var expID = 33554396;
   for( var i = 0; i < repeatNum - 4; i++ )
   {
      for( var j = 0; j < insertNum; j++ )
      {
         expIDs.push( expID );
         expID = expID + stringLength + recordHead;
      }

      //跨块时，加上块尾的空隙
      var expID = expID + 988;
   }

   //检查主备节点一致
   checkConsistency( db, csName, clName );

   //校验多个块内的_id值
   checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
   checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

   //检查记录数
   expectCount = expectCount - 32767;
   checkCount( dbcl, null, expectCount );
   println( "--check count success!" );

   //正向pop 3个块
   var skipNum = insertNum * 3 - 1;
   var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, skipNum );
   pop( dbcl, logicalID[0], 1 );

   //计算多个块内的预期的_id值
   var expIDs = [];
   var expID = 134217584;
   for( var i = 0; i < repeatNum - 7; i++ )
   {
      for( var j = 0; j < insertNum; j++ )
      {
         expIDs.push( expID );
         expID = expID + stringLength + recordHead;
      }

      //跨块时，加上块尾的空隙
      var expID = expID + 988;
   }

   //检查主备节点一致
   checkConsistency( db, csName, clName );

   //校验多个块内的_id值
   checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
   checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

   //检查记录数
   expectCount = expectCount - skipNum - 1;
   checkCount( dbcl, null, expectCount );
   println( "--check count success!" );

   commDropCS( db, csName, true, "drop CS in the end" );
   db1.close();
   db2.close();
}
main();
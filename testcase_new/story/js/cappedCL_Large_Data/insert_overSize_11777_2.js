/************************************
*@Description:insert data over Size
*@author:      zhaoyu
*@createdate:  2017.7.15
*@testlinkCase: seqDB-11777
**************************************/
//获取随机长度的字符串
var minLength = 1;
var maxLength = 16 * 1024;
var range = maxLength - minLength;
var recordHead = 55;
var stringLength = 1;
var recordLength = stringLength + recordHead;

//预期结果定义
var expIDs = [];
var expID = 0;
var nextExpID = 0;
var expectNum = 0;
var blockID = 1;

//最大值
var oneBlockMaxLID = 33554396;
var maxSize = 64;
var maxLogicalID = maxSize / 32 * oneBlockMaxLID;

var flag = true;

function main ()
{
   var csName = COMMCSNAME + "_11777_2";
   commDropCS( db, csName, true, "drop CS in the beginning" );

   var csOption = { Capped: true };
   commCreateCS( db, csName, false, "", csOption );

   var clName = COMMCLNAME + "_11777_2";
   var clOption = { Capped: true, Size: maxSize, AutoIndexId: false, OverWrite: false };
   var dbcl = commCreateCLByOption( db, csName, clName, clOption, true, true );

   //获取主备节点
   var db1 = new Sdb( db );
   db1.setSessionAttr( { PreferedInstance: "m" } );
   var dbclPrimary = db1.getCS( csName ).getCL( clName );
   var db2 = new Sdb( db );
   db2.setSessionAttr( { PreferedInstance: "s" } );
   var dbclSlave = db2.getCS( csName ).getCL( clName );

   insertDataOverSize( dbcl );

   //检查主备节点一致
   checkConsistency( db, csName, clName );

   //比较count结果
   checkCount( dbclPrimary, null, expectNum );
   checkCount( dbclSlave, null, expectNum );
   println( "--count success! expectNum: " + expectNum );

   //校验多个块内的_id值
   checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
   checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

   var repeatNum = 5;
   for( var j = 0; j < repeatNum; j++ )
   {
      //随机获取某条记录的logicalID
      var skipNum = Math.ceil( 1 + Math.random() * ( expectNum - 2 ) );
      println( "--skipNum:" + skipNum );
      var logicalID = getLogicalID( dbcl, null, null, { _id: 1 }, 1, skipNum );

      //随机设置pop方向
      if( ( parseInt( Math.random() * 10 ) ) % 2 === 0 )
      {
         var direction = -1;
      } else
      {
         var direction = 1;
      }

      //执行pop
      pop( dbcl, logicalID[0], direction );

      //比较count结果
      if( direction == -1 )
      {
         expectNum = skipNum;
         expID = logicalID[0];
         blockID = Math.floor( expID / oneBlockMaxLID ) + 1;
         println( "--maxLogicalID:" + maxLogicalID );
         expIDs.splice( skipNum );
         flag = true;
         insertDataOverSize( dbcl );

         //检查主备节点一致
         checkConsistency( db, csName, clName );

         //比较count结果
         checkCount( dbclPrimary, null, expectNum );
         checkCount( dbclSlave, null, expectNum );
         println( "--count success! expectNum: " + expectNum );

         //校验多个块内的_id值
         checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
         checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );
      } else
      {
         expectNum = expectNum - skipNum - 1;
         expIDs = expIDs.splice( skipNum + 1 );
         flag = true;

         //计算预期最大的_id值
         var popBlock = Math.floor( logicalID[0] / oneBlockMaxLID );
         //blockID = blockID + popBlock;
         maxLogicalID = maxLogicalID + popBlock * oneBlockMaxLID;
         println( "--popBlock:" + popBlock );
         println( "--blockID:" + blockID );
         println( "--maxLogicalID:" + maxLogicalID );
         insertDataOverSize( dbcl );

         //检查主备节点一致
         checkConsistency( db, csName, clName );

         //比较count结果
         checkCount( dbclPrimary, null, expectNum );
         checkCount( dbclSlave, null, expectNum );
         println( "--count success! expectNum: " + expectNum );

         //校验多个块内的_id值
         checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
         checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );

         dbcl.truncate();
         println( "--truncate cl success!" );

         flag = true;
         expIDs = [];
         expID = 0;
         nextExpID = 0;
         expectNum = 0;
         blockID = 1;
         maxLogicalID = maxSize / 32 * oneBlockMaxLID;
         insertDataOverSize( dbcl );

         //检查主备节点一致
         checkConsistency( db, csName, clName );

         //比较count结果
         checkCount( dbclPrimary, null, expectNum );
         checkCount( dbclSlave, null, expectNum );
         println( "--count success! expectNum: " + expectNum );

         //校验多个块内的_id值
         checkLogicalID( dbclPrimary, null, null, { _id: 1 }, -1, 0, expIDs );
         checkLogicalID( dbclSlave, null, null, { _id: 1 }, -1, 0, expIDs );
      }
   }

   commDropCS( db, csName, true, "drop CS in the end" );
   db1.close();
   db2.close();
}
main();

function insertDataOverSize ( dbcl )
{
   while( flag )
   {
      stringLength = Math.ceil( minLength + Math.random() * range );

      //计算不定长度记录的预期_id值
      recordLength = stringLength + recordHead;
      if( recordLength % 4 !== 0 )
      {
         recordLength += ( 4 - recordLength % 4 );
      }

      //生成不定长度的字符串记录
      var doc = new StringBuffer();
      doc.append( stringLength, "a" );
      var strings = doc.toString();

      //处理跨块的情况
      nextExpID = expID + recordLength;
      if( nextExpID <= maxLogicalID )
      {
         if( nextExpID / oneBlockMaxLID != blockID && blockID == Math.floor( nextExpID / oneBlockMaxLID ) )
         {
            expID = oneBlockMaxLID * blockID++;
            nextExpID = expID + recordLength;
            println( "expID:" + expID );
            println( "nextExpID:" + nextExpID );
            println( "recordLength:" + recordLength );
         }

         dbcl.insert( { a: strings } );
         expIDs.push( expID );
         expectNum++;

         //lid=expID的记录实际并未插入到集合中，也未添加到expIDs中
         //nextExpID = expID + recordLength;
         expID = nextExpID;
      } else
      {
         try
         {
            dbcl.insert( { a: strings } );
            println( "recordLength:" + recordLength );
            throw "NEED_ERROR";
         } catch( e )
         {
            flag = false;
            println( "--insert data up to limit!" );
            if( e !== -307 )
            {
               throw buildException( "insert data!", e, null, null, e );
            }
         }
      }
   }
   //println("expIDs:" + expIDs);
   println( "--blockID:" + blockID );

}
/******************************************************************************
@Description : seqDB-7463: hash自动切分，分区键为不同数据类型，验证数据分布规则
                           type: timestamp
@Author :
   2019-8-23   XiaoNi Huang  init
*******************************************************************************/
main();

function main ()
{
   if( true == commIsStandalone( db ) )
   {
      println( "---Is standalone." );
      return;
   }

   if( commGetGroupsNum( db ) < 2 )
   {
      println( "---Least two groups" );
      return;
   }

   var suffix = "_timestamp_7463";
   var dmName = "dm" + suffix;
   var csName = "cs" + suffix;
   var clName = "cl" + suffix;
   var groups = commGetGroups( db, false, "", false, true, true );
   var groupNames = [groups[1][0].GroupName, groups[2][0].GroupName];
   var cl;
   var recordsNum = getRandomInt( 1000, 5000 );
   println( "\nrecords number = " + recordsNum );

   commDropCS( db, csName, true, "drop cs in the begin" );
   commDropDomain( db, dmName, true, "drop domain in the end." );

   // random partition
   var partition = getRandomPartition();
   // create domain / cs / cl
   println( "\n---Begin to create domain & cs & cl, groups[ " + groupNames + " ]." );
   db.createDomain( dmName, groupNames, { "AutoSplit": true } );
   var cs = db.createCS( csName, { "Domain": dmName } );
   cl = cs.createCL( clName, { "ShardingType": "hash", "ShardingKey": { b: 1 }, "Partition": partition } );

   // insert
   var recordsArr = readyRdmRecs( recordsNum );
   println( "\n---Begin to insert records." );
   cl.insert( recordsArr );

   // check results
   checkRecs( cl, recordsArr );
   checkHashDistribution( groupNames, csName, clName, recordsNum );

   println( "\n---Begin to drop domain & cs." );
   commDropCS( db, csName, false, "drop cs in the end." );
   commDropDomain( db, dmName, false, "drop domain in the end." );
}

function checkRecs ( cl, expRecsArr )
{
   println( "\n---Begin to check records." );
   // check total count
   var cnt = Number( cl.count() );
   if( expRecsArr.length !== cnt )
   {
      throw buildException( "checkRecs", null, "[check number]", expRecsArr.length, cnt );
   }

   // check records
   var actRecsArr = [];
   var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( { a: 1 } );
   while( recs = cursor.next() )
   {
      actRecsArr.push( recs.toObj() );
   }

   if( JSON.stringify( expRecsArr ) !== JSON.stringify( actRecsArr ) )
   {
      throw buildException( "checkRecs", null, "[check records]",
         JSON.stringify( expRecsArr ),
         JSON.stringify( actRecsArr ) );
   }
}

function checkHashDistribution ( groupNames, csName, clName, expRecsNum )
{
   println( "\n---Begin to check hash distribution." );
   var rgDB1 = null;
   var rgDB2 = null;
   try 
   {
      rgDB1 = db.getRG( groupNames[0] ).getMaster().connect();
      rgDB2 = db.getRG( groupNames[1] ).getMaster().connect();
      var cnt1 = Number( rgDB1.getCS( csName ).getCL( clName ).count() );
      var cnt2 = Number( rgDB2.getCS( csName ).getCL( clName ).count() );

      // check total count on all groups
      var totalCnt = cnt1 + cnt2;
      if( expRecsNum !== totalCnt )
      {
         throw buildException( "checkHashDistribution", null, "[cl.count]", expRecsNum, totalCnt );
      }

      // check hash distribution, expect difference value is 0.3 of the total number of records
      var expDiffVal = expRecsNum * 0.3;
      var expRgRecsCnt = expRecsNum / groupNames;
      var actDiffVal1 = Math.abs( expRgRecsCnt - cnt1 );
      var actDiffVal2 = Math.abs( expRgRecsCnt - cnt2 );
      if( expDiffVal < actDiffVal1 || expDiffVal < actDiffVal2 )
      {
         throw buildException( "checkHashDistribution", null, "[hash distribution]",
            "[expDiffVal: " + expDiffVal + "]",
            "[actDiffVal1 : " + actDiffVal1 + ", actDiffVal2: " + "actDiffVal2" + "]" );
      }
   }
   finally 
   {
      if( rgDB1 !== null ) rgDB1.close();
      if( rgDB2 !== null ) rgDB2.close();
   }
}

function readyRdmRecs ( recordsNum, recordsArr ) 
{
   println( "\n---Begin to ready random records." );
   var recordsArr = [];
   for( var i = 0; i < recordsNum; i++ )
   {
      var time = getRandomStdTime();
      recordsArr.push( { "a": i, "b": { "$timestamp": time } } );
   }
   println( "sample records: " + JSON.stringify( recordsArr[0] ) );
   //println("records: " + JSON.stringify( recordsArr ));
   return recordsArr;
}

function getRandomStdTime ()
{
   // get YY
   var YY = "" + getRandomInt( 2000, 2037 );
   // get MM
   var MM = "" + getRandomInt( 1, 12 );
   while( MM.length < 2 )
   {
      MM = "0" + MM;
   }
   // get DD
   var DD = "" + getRandomInt( 1, 28 ); //february month max days: 28
   while( DD.length < 2 )
   {
      DD = "0" + DD;
   }
   // get HH 
   var HH = "" + getRandomInt( 0, 23 );
   while( HH.length < 2 )
   {
      HH = "0" + HH;
   }
   // get mm
   var mm = "" + getRandomInt( 0, 59 );
   while( mm.length < 2 )
   {
      mm = "0" + mm;
   }
   // get ss
   var ss = "" + getRandomInt( 0, 59 );
   while( ss.length < 2 )
   {
      ss = "0" + ss;
   }
   // get ffffff
   var ffffff = "" + getRandomInt( 0, 999999 ); //ffffff: [000000, 999999]
   while( ffffff.length < 6 )
   {
      ffffff = "0" + ffffff;
   }

   var date = YY + "-" + MM + "-" + DD + "-" + HH + "." + mm + "." + ss + "." + ffffff;
   return date;
}

function getRandomInt ( min, max )
{
   var rdmVal = min + Math.round( Math.random() * ( max - min ), max );
   return rdmVal;
}

function getRandomPartition ()
{
   var baseNum = 2;
   var powNum = getRandomInt( 3, 20 ); // [2^3, 2^20]
   var rdmPow = Math.pow( baseNum, powNum );
   return rdmPow;
}
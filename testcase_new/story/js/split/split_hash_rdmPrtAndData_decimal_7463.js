/******************************************************************************
@Description : seqDB-7463: hash自动切分，分区键为不同数据类型，验证数据分布规则
                           type: decimal
@Author :
   2019-8-23   XiaoNi Huang  init
*******************************************************************************/
//main(); 
/*CI跑失败(3g3d 1191)：
---Begin to ready expect records.
shell:4 uncaught exception: -152
Evalution failed with error
*/

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

   var suffix = "_decimal_7463";
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
   var expRecsArr = readyExpRecs( recordsArr );
   checkRecs( cl, expRecsArr );
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
      var rdmVal = getRandomNum( -1.7E+308, 1.7E+308 );
      recordsArr.push( { "a": i, "b": { "$decimal": String( rdmVal ) } } );
   }
   println( "sample records: " + JSON.stringify( recordsArr[0] ) );
   return recordsArr;
}

function readyExpRecs ( recordsArr )
{
   println( "\n---Begin to ready expect records." );
   var expRecsArr = [];
   for( var i = 0; i < recordsArr.length; i++ )  
   {
      var oriStr = recordsArr[i]["b"]["$decimal"];
      // get int, e.g:1.234E+10, get 1
      var intStr = oriStr.substring( 0, oriStr.indexOf( "." ) );
      // get decimal, e.g:1.234E+10, get 234
      var decimalStr = oriStr.substring( oriStr.indexOf( "." ) + 1, oriStr.indexOf( "e" ) );
      // get index number, e.g:1.234E+10, get 10
      var indexStr = oriStr.substring( oriStr.indexOf( "+" ) + 1, oriStr.length );
      //println("\noriStr  = "+ oriStr +"\nintStr  = "+ intStr +", decimalStr = "+ decimalStr +", indexStr = "+ indexStr ); 

      // e -> number, e.g: 1.234E+5, get 123400
      var numStr = "";
      if( decimalStr.length <= Number( indexStr ) )
      {
         // get number of zero, e.g: e.g: 1.234E+5 -> 123400, get number of zero is 2 (00)
         var zeroNum = Number( indexStr ) - decimalStr.length;
         var zeroStr = "";
         for( j = 0; j < zeroNum; j++ )
         {
            zeroStr += "0";
         }
         numStr = intStr + decimalStr + zeroStr;
         //println("zeroNum = "+ zeroNum +"\nnumStr  = "+ numStr ); 
      }

      // e -> number, e.g: 1.234E+2, get 123.4
      if( decimalStr.length > Number( indexStr ) )
      {
         // get int str, e.g: e.g: 1.234E+3 -> 123.4, get 123
         var newIntStr = intStr + decimalStr.substring( 0, tmpDecimalNum );
         // get decimal number, e.g: e.g: 1.234E+3 -> 123.4, get 1 ("4".length)
         var tmpDecimalNum = Number( indexStr ) - decimalStr.length;
         // get decimal str, e.g: 1.234E+3 -> 123.4, get 4
         var newDecimalStr = decimalStr.substring( tmpDecimalNum + 1, decimalStr.length );
         // get decimal, e.g: e.g: 1.234E+3 -> 123.4, get 123.4
         numStr = newIntStr + "." + newDecimalStr;
         //println("newIntStr = "+ newIntStr +"tmpDecimalNum = "+ tmpDecimalNum +", newDecimalStr = "+ newDecimalStr +"\nnumStr  = "+ numStr );
      }
      var newRecord = { "a": i, "b": { "$decimal": numStr } };
      expRecsArr.push( newRecord );
   }
   return expRecsArr;
}

function getRandomNum ( min, max )
{
   // get -x, decimal digits up to 15 digits, exceeding the value is inaccurate
   var rdmVal1 = ( Math.random() * min ).toFixed( 15 );
   // get +x, decimal digits up to 15 digits, exceeding the value is inaccurate
   var rdmVal2 = ( Math.random() * max ).toFixed( 15 );
   var rdmValArr = [rdmVal1, rdmVal2];
   // get -x or +x
   var tmpNum = Math.floor( Math.random() * rdmValArr.length );
   var rdmVal = rdmValArr[tmpNum];

   return rdmVal;
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
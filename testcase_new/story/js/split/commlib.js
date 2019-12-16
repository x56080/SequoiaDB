/******************************************************************************
*@Description : Public function for testing split.
*@Modify list :
*               2014-6-17  xiaojun Hu  Init
******************************************************************************/

var csName = COMMCSNAME;
var clName = COMMCLNAME;

// Get data group and split
function splitGroup ( db, csName, inserNum )
{
   try
   {
      var groups = new Array();
      groups = commGetGroups( db, "GroupName", "", true, true );
      var csGroup = new Array();
      csGroup = commGetCSGroups( db, csName );
      var dNum = groups.length - 1;
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      var conNum = inserNum / dNum;
      for( var i = 0; i < groups.length; ++i )
      {
         cl.split( csGroup[0], groups[i], { No: conNum } );
         conNum = conNum * 2;
      }
   }
   catch( e )
   {
      println( "Failed to split, rc = " + e );
      throw e;
   }
}

// Insert data to SequoiaDB
function insertData ( db, csName, clName, insertNum )
{
   if( undefined == insertNum ) { insertNum = 1000; }

   try
   {
      var doc = [];
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      for( var i = 0; i < insertNum; ++i )
      {
         var no = i;
         var user = "布斯" + i;
         var phone = 13700000000 + i;
         var compa1 = "MI" + i;
         var compa2 = "GOLDEN" + i;
         var date = new Date();
         var time = date.getTime();
         var card = 6800000000 + i;
         /**********************************************************************
         data expampl : {"No":5, customerName:"布斯5","phoneNumber":13700000005,
                         "company":[MI5,GOLDEN5],"openDate":1402990912105,
                         "cardID":6800000005}
         **********************************************************************/
         //println( "Start to deal string" ) ;
         var insertString = "{\"No\":" + no + ",\"customerName\":\"" + user +
            "\",\"phoneNumber\":" + phone +
            ",\"company\":[\"" + compa1 + "\",\"" + compa2 +
            "\"],\"openDate\":" + time + ",\"cardID\":" + card + "}";
         //println( "String + " + insertString ) ;
         var insert = eval( "(" + insertString + ")" );
         doc.push( insert );
      }
      cl.insert( doc );

   }
   catch( e )
   {
      println( "Failed to insert data to Sdb, rc = " + e );
      throw e;
   }
   var cmd = new Cmd();
   var info = cmd.run( "date" );
   println( "insert endtime is " + info );
}

// Query the data
function queryData ( db, csName, clName )
{
   try
   {
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      /**********************************************************************
      data expampl : {"No":5, customerName:"布斯5","phoneNumber":13700000005,
                      "company":[MI5,GOLDEN5],"openDate":1402990912105,
                      "cardID":6800000005}
      query data and get quantity.
      **********************************************************************/
      db.setSessionAttr( { PreferedInstance: "M" } );
      var query =
         cl.find( {
            $and: [{
               "No": { $gte: 50 }, "phoneNumber": { $lt: 13700001000 },
               "customerName": { $nin: ["MI500", "GOLDEN500"] },
               "cardID": { $lte: 6800001111 },
               "openDate": { $gt: 1402990912105 }
            }]
         } ).count();
      if( 950 != query )
      {
         println( "Wrong query the data, count = " + query );
         throw "ErrQueryNum";
      }
      println( "Success to query the data" );
   }
   catch( e )
   {
      println( "Failed to query data, rc = " + e );
      throw e;
   }
   var cmd = new Cmd();
   var info = cmd.run( "date" );
   println( "query endtime is " + info );
}

// Update the data
function updateData ( db, csName, clName )
{
   try
   {
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      /**********************************************************************
      data expampl : {"No":5, customerName:"布斯5","phoneNumber":13700000005,
                      "company":[MI5,GOLDEN5],"openDate":1402990912105,
                      "cardID":6800000005}
      query data and get quantity.
      **********************************************************************/
      cl.update( {
         $inc: { "cardID": 5 }, $set: { "customerName": "布斯" },
         $unset: { "openDate": "" }, $addtoset: { company: [2, 3] },
         $pull_all: { "company": ["MI88", "GOLDEN88", 2, 3] }
      } );
      sleep( 20 );
      var count = cl.find( {
         "No": 88, "customerName": "布斯",
         "phoneNumber": 13700000088, "cardID": 6800000093,
         "company": ["MI88", "GOLDEN88", 2, 3]
      } ).count();
      if( 1 != count )
      {
         println( "Wrong update the data, count = " + count );
         throw "ErrUpdateData";
      }
      println( "Success to update the data." );
   }
   catch( e )
   {
      println( "Failed to update the data, rc = " + e );
      throw e;
   }
   var cmd = new Cmd();
   var info = cmd.run( "date" );
   println( "update time is " + info );
}

// Remove data
function removeData ( db, csName, clName )
{
   try
   {
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );
      /**********************************************************************
      data expampl : {"No":5, customerName:"布斯5","phoneNumber":13700000005,
                      "company":[MI5,GOLDEN5],"openDate":1402990912105,
                      "cardID":6800000005}
      query data and get quantity.
      **********************************************************************/
      cl.remove( { "No": { $gte: 89 } }, { "": "$id" } );
      var cnt = 0;
      //sleep( 20 ) ;
      //do
      //{
      var sleepInteval = 2;
      var sleepDuration = 0;
      var maxSleepDuration = 600000;
      var count = cl.count();
      //if( 84 == count )
      // break ;
      //++cnt ;
      //}//while( cnt <= 20 ) 
      while( 89 !== Number( count ) && sleepDuration < maxSleepDuration )
      {
         sleep( sleepInteval );
         sleepDuration += sleepInteval;
      }
      println( "sleepDuration=" + sleepDuration );
      if( 89 !== Number( count ) )
      {
         println( "Wrong remove the date, count = " + count );
         throw "ErrRemoveData";
      }
      println( "Success to remove the data." );
   }
   catch( e )
   {
      println( "Failed to remove the data, rc = " + e );
      throw e;
   }

   var cmd = new Cmd();
   var info = cmd.run( "date" );
   println( "remove time is " + info );
}

// Get source group and destination group
function getTwoGroupSplit ( db, csName, clName, splitArg1, splitArg2 )
{
   try
   {
      // get collection
      var cs = db.getCS( csName );
      var cl = cs.getCL( clName );

      var listGroups = db.listReplicaGroups();
      var listGroupsArr = new Array();

      // Check over arguement "splitArg1" "splitArg2"
      if( "" == splitArg1 || undefined == splitArg1 )
      {
         println( "Wrong argument." );
         throw "ErrArg";
      }

      // argument : when the split is percent
      var argument = "";
      if( undefined == splitArg2 || "" == splitArg2 ) { argument = splitArg1; }
      // Get group where Collection Space located in
      while( listGroups.next() )
      {
         if( listGroups.current().toObj()["GroupID"] >= DATA_GROUP_ID_BEGIN )
         {
            listGroupsArr.push( listGroups.current().toObj()["GroupName"] );
         }
      }
      var groupNum = listGroupsArr.length;

      var snapShotCl = db.snapshot( SDB_SNAP_COLLECTIONS );
      var snapShotClName = new Array();
      var snapShotClGroup = new Array();
      var group = "";
      while( snapShotCl.next() )
      {
         snapShotClName.push( snapShotCl.current().toObj()["Name"] );
         snapShotClGroup.push( snapShotCl.current().toObj()["Details"][0]["GroupName"] );
      }
      var clname = csName + "." + clName;
      for( var i = 0; i < snapShotClGroup.length; i++ )
      {
         if( snapShotClName[i] == clname )
         {
            group = snapShotClGroup[i];
            break;
         }
      }
      if( "" == group )
      {
         println( "Failed to get Group where CL located in, snapshotCl = "
            + snapShotCl );
         throw "ErrGetGroup";
      }
      println( "The source group = " + group );
      // Get the other group where split to
      var groupSplit = "";
      var i = 0;
      do
      {
         if( group != listGroupsArr[i] )
         {
            groupSplit = listGroupsArr[i];
            break;
         }
         ++i;

      } while( i <= groupNum || i <= 8 );
      if( "" == groupSplit )
      {
         println( "Failed to get Split Group, Groups = " + listGroups );
         throw "ErrGetSplitGroup";
      }
      println( "The destination [split]group = " + groupSplit );
      //println( "Argument : " + argument ) ;

      var sleepInteval = 1;
      var sleepDuration = 0;
      var maxSleepDuration = 600000;

      if( "" == argument )     
      {
         cl.splitAsync( group, groupSplit, splitArg1, splitArg2 );
      }
      else
         cl.splitAsync( group, groupSplit, argument );

      var testcl = csName + "." + clName;

      while( db.listTasks( { "Name": testcl } ).next() && sleepDuration < maxSleepDuration )
      {
         sleep( sleepInteval );
         sleepDuration += sleepInteval;
      }
      println( "splittime=" + sleepDuration );
      println( "Success to Split" );
   }
   catch( e )
   {
      println( "Failed to get the group " + e );
      throw e;
   }
}

// Get group from Sdb
function getGroup ( db )
{
   try
   {
      var listGroups = db.listReplicaGroups();
      var groupArray = new Array();
      while( listGroups.next() )
      {
         if( listGroups.current().toObj()["GroupID"] >= DATA_GROUP_ID_BEGIN )
         {
            groupArray.push( listGroups.current().toObj()["GroupName"] );
         }
      }
      return groupArray;
   }
   catch( e )
   {
      println( "Failed to get groups from sdb, rc = " + e );
      throw e;
   }
}

// get group name [csName.clName] locate in
// 2015-12-19 Ting YU modify
function getPG ( csName, clName )
{
   try
   {
      var clFullName = csName + "." + clName;
      var clInfo = db.snapshot( 8, { Name: clFullName } );
      while( clInfo.next() )
      {
         var clInfoObj = clInfo.current().toObj();
         var srcGroupName = clInfoObj.CataInfo[0].GroupName;
      }
      println( csName + "." + clName + "'s source group: " + srcGroupName );

      return srcGroupName;
   }
   catch( e )
   {
      println( "getPG(): failed to get source group, cl: " + clFullName );
      throw e;
   }
}

//*****get GroupName ,return array**************
//eg : arrGroupName[?][0] == "GroupName"
//     arrGroupName[?][1] == "GroupID"
function getGroupName ( db )
{
   try
   {
      var RGname = db.listReplicaGroups().toArray();
   }
   catch( e )
   {
      throw e;
   }
   var j = 0;
   var arrGroupName = Array();
   for( var i = 1; i != RGname.length; ++i )
   {
      var eRGname = eval( '(' + RGname[i] + ')' );
      if( 1000 <= eRGname["GroupID"] )
      {
         arrGroupName[j] = Array();
         arrGroupName[j].push( eRGname["GroupName"] );
         arrGroupName[j].push( eRGname["Group"][0]["Service"][0]["Name"] );
         ++j;
      }
   }
   return arrGroupName;
}

//*****get GroupName ,return array**************
//eg : arrGroupName[?][0] == "GroupName"
//eg : arrGroupName[?][1] == "HostName"
//     arrGroupName[?][2] == "Service"
function getGroupName2 ( db, mustBePrimary )
{
   var RGname = null;
   try
   {
      RGname = db.listReplicaGroups().toArray();
   }
   catch( e )
   {
      throw e;
   }
   var j = 0;
   var arrGroupName = Array();
   for( var i = 1; i != RGname.length; ++i )
   {
      var eRGname = eval( '(' + RGname[i] + ')' );
      if( 1000 <= eRGname["GroupID"] )
      {
         arrGroupName[j] = Array();
         var primaryNodeID = eRGname["PrimaryNode"];
         var groups = eRGname["Group"];
         for( var m = 0; m < groups.length; m++ )
         {
            if( true == mustBePrimary )
            {
               var nodeID = groups[m]["NodeID"];
               if( primaryNodeID != nodeID )
                  continue;
            }
            arrGroupName[j].push( eRGname["GroupName"] );
            arrGroupName[j].push( groups[m]["HostName"] );
            arrGroupName[j].push( groups[m]["Service"][0]["Name"] );
            break;
         }
         ++j;
      }
   }
   return arrGroupName;
}

/************************************
*@Description: get the informations of the srcGroups and targetGroups,then split cl with different options,
               only split 1 times
               return the informations of the srcGroups and targetGroups
*@author：Yan Wu 2015/10/26
*@parameters:
             startCondition:start condition of split,if the typeof is number,then percentage split,if the typeof is object, 
             then range split
             endCondition:object of end condition
*@return array[][] ex:
        [0]
           {"GroupName":"XXXX"}
           {"HostName":"XXXX"}
           {"svcname":"XXXX"}           
        [N]            
**************************************/
function ClSplitOneTimes ( csName, clName, startCondition, endCondition )
{
   try
   {
      var targetGroupNums = 1;
      var groupsInfo = getSplitGroups( csName, clName, targetGroupNums );
      var srcGrName = groupsInfo[0].GroupName;
      var tarGrName = groupsInfo[1].GroupName;
      println( "tarGrName:" + tarGrName );
      var CL = db.getCS( csName ).getCL( clName );
      println( "--begin split" )
      if( typeof ( startCondition ) === "number" ) //percentage split
      {
         CL.split( srcGrName, tarGrName, startCondition );
      }
      else if( typeof ( startCondition ) === "object" && endCondition === undefined ) //range split without end condition
      {
         CL.split( srcGrName, tarGrName, startCondition );
         println( "startCondition=" + startCondition )
      }
      else if( typeof ( startCondition ) === "object" && typeof ( endCondition ) === "object" ) //range split with end condition
      {
         CL.split( srcGrName, tarGrName, startCondition, endCondition );
      }
      println( "--end split" )
   }
   catch( e )
   {
      throw e;
   }
   return groupsInfo;
}

/************************************
*@Description: get SrcGroup and TargetGroup info,the groups information
               include GroupName,HostName and svcname
*@author：wuyan 2015/10/14
*@return array[][] ex:
        [0]
           {"GroupName":"XXXX"}
           {"HostName":"XXXX"}
           {"svcname":"XXXX"}           
        [N]
           ...
**************************************/
function getSplitGroups ( csName, clName, targetGrMaxNums )
{
   //var allGroupInfo = commGetGroups(db, true) 
   var allGroupInfo = getGroupName2( db, true )
   var srcGroupName = getSrcGroup( csName, clName );
   var splitGroups = new Array();
   if( targetGrMaxNums >= allGroupInfo.length - 1 ) 
   {
      targetGrMaxNums = allGroupInfo.length - 1;
   }
   var index = 1;

   for( var i = 0; i != allGroupInfo.length; ++i )
   {
      if( srcGroupName == allGroupInfo[i][0] )
      {
         splitGroups[0] = new Object();
         splitGroups[0].GroupName = allGroupInfo[i][0];
         splitGroups[0].HostName = allGroupInfo[i][1];
         splitGroups[0].svcname = allGroupInfo[i][2];
         println( "splitGroups[0].GroupName=" + splitGroups[0].GroupName )
      }
      else 
      {
         if( index > targetGrMaxNums )
         {
            continue;
         }
         splitGroups[index] = new Object();
         splitGroups[index].GroupName = allGroupInfo[i][0];
         splitGroups[index].HostName = allGroupInfo[i][1];
         splitGroups[index].svcname = allGroupInfo[i][2];
         index++;
      }
   }
   return splitGroups;

}

/************************************
*@Description: get SrcGroup name,update getPG to getSrcGroup
*@author：wuyan 2015/10/14
**************************************/
function getSrcGroup ( csName, clName )
{
   try
   {
      if( undefined == csName || undefined == clName )
      {
         println( "cs name: " + csName + ", clName: " + clName );
         throw "cs or cl name is undefined";
      }
      var tableName = csName + "." + clName;
      //var catadb = new Sdb(COORDHOSTNAME,CATASVCNAME) ;
      var Group = db.snapshot( 8 ).toArray();
      var srcGroupName;
      for( var i = 0; i < Group.length; ++i )
      {
         var eachID = eval( "(" + Group[i] + ")" );
         if( tableName == eachID["Name"] )
         {
            srcGroupName = eachID["CataInfo"][0]["GroupName"];
            println( csName + "." + clName + "'s source group: " + srcGroupName );
            break;
         }
      }
      return srcGroupName;
   }
   catch( e )
   {
      println( "failed to get source group, cs name: " + csName +
         ", cl name: " + clName );
      throw e;
   }
}

/************************************
*@Description: create cl by split
*@author：wuyan 2016/3/11
**************************************/
function createSplitCl ( clName, shardingKey, shardingType )
{
   var options = { ShardingKey: shardingKey, ShardingType: shardingType, ReplSize: 0, Compressed: true };
   var varCL = commCreateCL( db, COMMCSNAME, clName, options, true, true );
   return varCL;
}

/************************************
*@Description: random generate splitKeys
*@author：wuyan 2016/3/18
*@usage:   eg:
          var splitKeys = generateSplitKeys( rd, "int","test" )
**************************************/
function generateSplitKeys ( rd, dataType, fieldName )
{
   try
   {
      var splitKeys = [];
      //var rd = new commDataGenerator();
      var data1 = rd.getValue( dataType );
      var data2 = rd.getValue( dataType );
      if( data1 === data2 )
      {
         data2 = rd.getValue( dataType );
      }

      if( data1 > data2 )
      {
         var beginStr = data2;
         var endStr = data1;
      }
      else
      {
         var beginStr = data1;
         var endStr = data2;
      }

      var beginKey = {};
      beginKey[fieldName] = beginStr;
      var endKey = {};
      endKey[fieldName] = endStr;
      splitKeys.push( beginKey );
      splitKeys.push( endKey );
      println( "range beginstr:" + beginStr );
      println( "range endstr:  " + endStr );

   }
   catch( e )
   {
      throw buildException( "generateSplitKeys()", e );
   }
   return splitKeys;
}

/************************************
*@Description: check the split resutl
*@author：wuyan 2016/3/18
**************************************/
function checkSplitResult ( clName, dataNodeInfo, fieldName, splitKeys, docs )
{
   var beginKey = splitKeys[0][fieldName];
   var endKey = splitKeys[1][fieldName];

   var total = 0;
   var gdb0 = new Sdb( dataNodeInfo[0].HostName, dataNodeInfo[0].svcname );
   var gdb1 = new Sdb( dataNodeInfo[1].HostName, dataNodeInfo[1].svcname );
   try
   {
      for( var i = 0; i != docs.length; ++i )
      {
         if( docs[i][fieldName] >= beginKey && docs[i][fieldName] < endKey )
         {
            var notExistNum = gdb0.getCS( COMMCSNAME ).getCL( clName ).find( docs[i] ).count();
            var existNum = gdb1.getCS( COMMCSNAME ).getCL( clName ).find( docs[i] ).count();
            total += existNum;
         }
         else if( docs[i][fieldName] < beginKey || docs[i][fieldName] >= endKey )
         {
            var existNum = gdb0.getCS( COMMCSNAME ).getCL( clName ).find( docs[i] ).count();
            var notExistNum = gdb1.getCS( COMMCSNAME ).getCL( clName ).find( docs[i] ).count();
            total += existNum;
         }
         else
         {
            throw buildException( "the data is not in srcGroups and targetGroups", docs[i][fieldName] );
         }

         if( existNum !== 0 && notExistNum === 0 )
         {
            throw buildException( "checkSplitResult()", "targetGroup data range wrong", "compare targetGroup data", "range is ok", objMax[fieldName] )
         }
      }
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      closeDB( gdb0 );
      closeDB( gdb1 );
   }

   //check datas num of the split groups
   if( Number( total ) !== docs.length )			
   {
      throw buildException( "checkClSplitResult()", "count wrong", "count()", docs.length, total )
   }
}

/************************************
*@Description: check the split resutl,the datatype is json ,eg.date/time/oid......（只比对数据总数，数据内容比较后续再更新）
*@author：wuyan 2016/3/18
**************************************/
function checkSplitResultBackup ( clName, dataNodeInfo, insertNum )
{
   var total = 0;
   for( var i = 0; i != dataNodeInfo.length; ++i )
   {
      try
      {
         var gdb = new Sdb( dataNodeInfo[i].HostName, dataNodeInfo[i].svcname );
         var cl = gdb.getCS( COMMCSNAME ).getCL( clName )
         var num = cl.find().count();
         total += num;
      }
      catch( e )
      {
         throw e;
      }
      finally
      {
         if( gdb !== undefined )
         {
            gdb.close();
            gdb == undefined;
         }
      }
   }
   //check datas num of the split groups
   if( Number( total ) !== insertNum )			
   {
      throw buildException( "checkClSplitResult()", "count wrong", "count()", insertNum, total )
   }
}

function closeDB ( sdb )
{
   if( undefined !== sdb )
   {
      sdb.close();
      sdb = undefined;
   }
}
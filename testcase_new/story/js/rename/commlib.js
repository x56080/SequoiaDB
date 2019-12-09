/************************************
*@Description: insert data
*@author:      wuyan
*@createDate:  2018.1.22
**************************************/
function insertData ( dbcl, number )
{
   if( undefined == number ) { number = 1000; }
   try
   {
      println( "---begin to insert data " );
      var docs = [];
      for( var i = 0; i < number; ++i )
      {
         var no = i;
         var a = i;
         var user = "test" + i;
         var phone = 13700000000 + i;
         var time = new Date().getTime();
         var doc = { no: no, a: a, customerName: user, phone: phone, openDate: time };
         //data example: {"no":5, customerName:"test5", "phone":13700000005, "openDate":1402990912105

         docs.push( doc );
      }
      dbcl.insert( docs );
   }
   catch( e )
   {
      throw buildException( "insertData()", e, "insert", "insert success", "insert fail" );
   }
}

/************************************
*@Description: create file by lob
*@author:      wuyan
*@createDate:  2018.10.12
**************************************/
function createFile ( fileName, fileSize )
{
   if( undefined == fileSize ) { fileSize = "100K"; }
   var cmd = new Cmd();
   var str = "dd if=/dev/zero of=" + fileName + " bs=" + fileSize + " count=1";
   cmd.run( str );

   var md5 = cmd.run( "md5sum " + fileName ).split( " " )[0];
   return md5;
}

/************************************
*@Description: delete file
*@author:      luweikang
*@createDate:  2018.11.06
**************************************/
function deleteFile ( fileName )
{
   var cmd = new Cmd();
   cmd.run( "rm -rf " + fileName );
}

/************************************
*@Description: put lobs
*@author:      wuyan
*@createDate:  2018.10.12
**************************************/
function putLobs ( cl, fileName, lobNum )
{
   if( undefined == lobNum ) { lobNum = 1; }

   println( "---begin to put lob " );
   var lobIdArr = [];
   for( var i = 0; i < lobNum; i++ )
   {
      var lobId = cl.putLob( fileName );
      lobIdArr.push( lobId );
   }
   return lobIdArr;
}

/************************************
*@Description: delete the lob
*@author:      wuyan
*@createDate:  2018.10.12
**************************************/
function deleteLobs ( cl, lobIdArr )
{
   println( "\n---begin to deleteLobs " );

   for( var i in lobIdArr )
   {
      cl.deleteLob( lobIdArr[i] );
   }
}


/************************************
*@Description: check the lob
*@author:      wuyan
*@createDate:  2018.10.12
**************************************/
function checkLob ( cl, expLobArr, srcMd5 )
{
   println( "---begin to checkLob " );

   var rc = cl.listLobs();

   //check Available
   var lobArr = [];
   while( rc.next() )
   {
      var lobInfo = rc.current().toObj();
      var lobId = lobInfo["Oid"]["$oid"];
      var isNormal = lobInfo["Available"];
      lobArr.push( lobId );

      if( isNormal !== true )
      {
         println( "lobId=" + lobId );
         throw buildException( "check Available", null, "cl.listLobs()",
            '"Available":true', '"Available":' + isNormal );
      }
   }

   //check lob number 
   if( lobArr.length !== expLobArr.length )
   {
      throw buildException( "check lob number", null, "cl.listLobs()",
         'lob number:' + expLobArr.length,
         'lob number:' + lobArr.length );
   }

   lobArr.sort();
   expLobArr.sort();
   //check lob Id
   for( var i in expLobArr )
   {
      if( lobArr[i] !== expLobArr[i] )
      {
         throw buildException( "check lob Id", null, "cl.listLobs()",
            'lob Id:' + expLobArr[i],
            'lob Id:' + lobArr[i] );
      }
   }

   //get lob and check md5sum
   var fileName = CHANGEDPREFIX + "_lobtest_getlob.file";
   for( var i in lobArr )
   {
      var lobId = lobArr[i];
      cl.getLob( lobId, fileName, true );

      var cmd = new Cmd();
      var desMd5 = cmd.run( "md5sum " + fileName ).split( " " )[0];
      if( desMd5 !== srcMd5 )
      {
         throw buildException( "get lob and check md5sum", null, "md5sum " + fileName,
            srcMd5, desMd5 );
      }

      cmd.run( "rm -rf " + fileName );
   }
}


/************************************
*@Description: check the new cl name 
*@author:      wuyan
*@createDate:  2018.10.12
**************************************/
function checkRenameCLResult ( csName, oldCLName, newCLName )
{
   try
   {
      var clFullName = csName + "." + newCLName;
      var getNewCLName = db.snapshot( SDB_SNAP_COLLECTIONS, { "Name": clFullName } ).current().toObj().Name;
      if( getNewCLName !== clFullName )
      {
         throw buildException( "check cl name", null, "check the new cl name",
            clFullName, getNewCLName );
      }

      //check the old cl is not exist
      try
      {
         db.getCS( csName ).getCL( oldCLName );
         throw "need throw error";
      }
      catch( e )
      {
         if( e !== -23 )
         {
            throw buildException( "check old clName:", e );
         }
      }
   }
   catch( e )
   {
      throw buildException( "checkRenameCLResult", e )
   }
}

/************************************
*@Description: check the new maincl name 
*@author:      wuyan
*@createDate:  2018.10.12
**************************************/
function checkRenameMainCLResult ( maincs, subcs, newMainCLName, oldMainCLName, subclName )
{
   try
   {
      var subclFullName = subcs + "." + subclName;
      var newMainCLFullName = maincs + "." + newMainCLName;
      var getMainCLName = db.snapshot( 8, { "Name": subclFullName } ).current().toObj().MainCLName;
      if( getMainCLName !== newMainCLFullName )
      {
         throw buildException( "check mainclName", null, "check the new maincl name",
            newMainCLFullName, getMainCLName );
      }

      //check the old maincl is not exist
      try
      {
         db.getCS( maincs ).getCL( oldMainCLName );
         throw "need throw error";
      }
      catch( e )
      {
         if( e !== -23 )
         {
            throw buildException( "check old mianclName:", e );
         }
      }
   }
   catch( e )
   {
      throw buildException( "checkRenameMainCLResult", e )
   }
}

/************************************
*@Description: get group name and service name .
*@author:      wuyan
*@createDate:  2018/10/12
**************************************/
function getGroupName ( db, mustBePrimary )
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
*@Description: get SrcGroup name,update getPG to getSrcGroup
*@author��wuyan 2018/10/12
**************************************/
function getSrcGroup ( csName, clName )
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
      return srcGroupName;
   }
   catch( e )
   {
      println( "failed to get source group, cl name: " + clFullName );
      throw e;
   }
}

/************************************
*@Description: get SrcGroup and TargetGroup info,the groups information
               include GroupName,HostName and svcname
*@author��wuyan 2018/10/12
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
   var allGroupInfo = getGroupName( db, true );
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

function splitCL ( dbcl, srcGroupName, dstGroupName )
{
   try
   {
      var percent = 50;
      dbcl.split( srcGroupName, dstGroupName, percent );
   }
   catch( e )
   {
      throw buildException( "splitcl", null, "split fail!",
         srcGroupName, dstGroupName + "  e:" + e );
   }
}

/************************************
*@Description: create maincl .
*@author:      wuyan
*@createDate:  2018/10/12
**************************************/
function createMainCL ( csName, mainCLName, shardingKey )
{
   println( "---begin to create MainCL." );

   var options = { ShardingKey: shardingKey, IsMainCL: true, ReplSize: 0 };
   var mainCL = commCreateCLByOption( db, csName, mainCLName, options, false,
      true, "Failed to create mainCL." );
   return mainCL;
}

function createCL ( csName, clName, shardingKey, shardingType )
{
   if( typeof ( shardingType ) == "undefined" ) { shardingType = "hash"; }
   println( "---begin to create cl:" + csName + "." + clName );

   var options = { ShardingKey: shardingKey, ShardingType: shardingType, ReplSize: 0, Compressed: true };
   var dbcl = commCreateCLByOption( db, csName, clName, options, true,
      true, "Failed to create cl." );
   return dbcl;
}


/************************************
*@Description: check the new cs name 
*@author:      luweikang
*@createDate:  2018.10.13
**************************************/
function checkRenameCSResult ( oldCSName, newCSName, clNum )
{
   println( "---begin to check cs: " + newCSName );
   if( undefined == clNum ) { clNum = 1; }
   //max cycle
   var maxTime = 5000;
   //current cycle
   var currentTime = 0;
   //sleep time��100 millisecond
   var intervalTime = 100;

   //because some dataNode none complete sync��so add retry for 5 seconds
   var clArray = getCSSnapshotCLArray( newCSName );
   while( clArray.length !== clNum && currentTime < maxTime )
   {
      sleep( 100 );
      currentTime += intervalTime;
      clArray = getCSSnapshotCLArray( newCSName );
   }
   if( currentTime === maxTime )
   {
      throw buildException( "check cl num time out, it took five seconds ", null, JSON.stringify( clArray ),
         clNum, clArray.length );
   }

   //when the cl num expected results are met��check the cl name
   for( i = 0; i < clArray.length; i++ )
   {
      var csname = clArray[i].Name.split( "." )[0];
      if( csname !== newCSName )
      {
         throw buildException( "check cs.cl name", null, JSON.stringify( newCSObj ),
            newCSName, csname );
      }
   }

   //check the old cl is not exist
   try
   {
      db.getCS( oldCSName );
      throw "CS_IS_EXIT";
   }
   catch( e )
   {
      if( e !== -34 )
      {
         throw buildException( "check old csName:", e );
      }
   }
}

function getCSSnapshotCLArray ( newCSName )
{
   var newCSObj = db.snapshot( SDB_SNAP_COLLECTIONSPACES, { "Name": newCSName } ).current().toObj();
   var getNewCSName = newCSObj.Name;
   if( getNewCSName !== newCSName )
   {
      throw buildException( "check cs name", null, "check the new cs name",
         newCSName, getNewCSName );
   }

   var clArray = newCSObj.Collection;
   return clArray;
}

function checkRenameSubCLResult ( maincs, mainCLName, subcs, oldSubCLName, newSubCLName )
{
   try
   {
      println( "---begin to check the rename Subcl name" );
      var newSubCLFullName = subcs + "." + newSubCLName;
      var mainCLFullName = maincs + "." + mainCLName;
      var subCLInfo = db.snapshot( 8, { "Name": newSubCLFullName } ).current().toObj();
      var getMainCLName = subCLInfo.MainCLName;
      if( getMainCLName !== mainCLFullName )
      {
         throw buildException( "check maincl Name", null, "check the new maincl name",
            mainCLFullName, getMainCLName );
      }

      var getSubCLName = subCLInfo.Name;
      if( getSubCLName !== newSubCLFullName )
      {
         throw buildException( "check new subclName", null, "check the new subcl name",
            newSubCLFullName, getSubCLName );
      }

      //check the old subcl is not exist
      try
      {
         db.getCS( subcs ).getCL( oldSubCLName );
         throw "need throw error";
      }
      catch( e )
      {
         if( e !== -23 )
         {
            throw buildException( "check old subclName:", e );
         }
      }
   }
   catch( e )
   {
      throw buildException( "checkRenameSubCLResult", e )
   }
}
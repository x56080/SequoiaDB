/*******************************************************************************
@Description : alter common functions
@Modify list :
2018-4-25  wuyan  Init
*******************************************************************************/

//inspect the alter field is success or not.
function checkAlterResult ( clName, fieldName, expFieldValue, csName )
{
   if( csName == undefined ) { csName = COMMCSNAME; }
   var clFullName = csName + "." + clName;
   var cur = db.snapshot( 8, { "Name": clFullName } );
   var actualFieldValue = cur.current().toObj()[fieldName];

   if( typeof ( expFieldValue ) === "object" )
   {
      if( JSON.stringify( expFieldValue ) !== JSON.stringify( actualFieldValue ) )
      {
         throw buildException( "test fieldvalue1", "check field", "value is wrong", JSON.stringify( expFieldValue ), JSON.stringify( actualFieldValue ) );
      }

   }
   else
   {
      if( expFieldValue !== actualFieldValue )
      {
         throw buildException( "test fieldvalue2", "check field", "value is wrong", expFieldValue, actualFieldValue );
      }
   }

}

//inspect the alter cs field is success or not.
function checkAlterCSResult ( csName, fieldName, expFieldValue )
{
   try
   {
      var rg = db.getRG( "SYSCatalogGroup" );
      var dbca = new Sdb( rg.getMaster() );
      var cur = dbca.SYSCAT.SYSCOLLECTIONSPACES.find( { "Name": csName } );
      while( cur.next() )
      {
         var tempinfo = cur.current().toObj();
         var actFieldValue = tempinfo[fieldName];
      }

      if( expFieldValue !== actFieldValue )
      {

         println( "---" + expFieldValue );
         throw buildException( "test fieldvalue", "check field", "", expFieldValue, actFieldValue );
      }

   }
   catch( e )
   {
      throw buildException( "check alter cs result:", e );
   }
   finally
   {
      if( dbca != null )
      {
         dbca.close()
      }
   }
}


/************************************
*@Description: check snapshot
*@author:      luweikang
*@createDate:  2018.4.25
**************************************/
function checkSnapshot ( db, snapType, csName, clName, field, expFieldValue )
{
   var clFullName = csName + "." + clName;
   var cursor = db.snapshot( snapType, { 'Name': clFullName } );
   var Obj = cursor.current().toObj();
   var actualFieldValue = Obj[field];
   println( "expFieldValue   : " + expFieldValue );
   println( "actualFieldValue: " + actualFieldValue );
   if( typeof ( expFieldValue ) === "object" )
   {
      if( JSON.stringify( expFieldValue ) !== JSON.stringify( actualFieldValue ) )
      {
         throw buildException( "test fieldvalue1", "check field", "value is wrong", JSON.stringify( expFieldValue ), JSON.stringify( actualFieldValue ) );
      }

   }
   else
   {
      if( expFieldValue !== actualFieldValue )
      {
         throw buildException( "test fieldvalue2", "check field", "value is wrong", expFieldValue, actualFieldValue );
      }
   }

}

/************************************
*@Description: get all groupName
*@author:      luweikang
*@createDate:  2018.4.25
**************************************/
function getGroupName ( db )
{
   var groupArr = db.listReplicaGroups();
   var dataRGNames = new Array();
   while( groupArr.next() )
   {
      var groupObj = groupArr.current().toObj();
      var groupID = groupObj.GroupID;
      if( groupID >= 1000 )
      {
         dataRGNames.push( groupObj.GroupName );
      }
   }
   return dataRGNames;
}

/************************************
*@Description: get Split Group
*@author:      luweikang
*@createDate:  2018.4.25
**************************************/
function getSplitGroup ( db, csName, clName )
{
   var clFullName = csName + "." + clName;
   var arr = getGroupName( db );
   var srcGroup = commGetCLGroups( db, clFullName )[0];
   var tarGroup = null;
   for( i in arr )
   {
      if( arr[i] !== srcGroup )
      {
         tarGroup = arr[i];
         break;
      }
   }
   var splitGroup = {};
   splitGroup.srcGroup = srcGroup;
   splitGroup.tarGroup = tarGroup;
   return splitGroup;
}

/************************************
*@Description: alter cl
*@author:      luweikang
*@createDate:  2018.4.25
**************************************/
function clSetAttributes ( cl, options )
{
   try
   {
      cl.setAttributes( options );
      throw "ALTER_SHOULD_ERR";
   }
   catch( e )
   {
      if( e !== -32 )
      {
         throw e;
      }
   }
}

/* *****************************************************************************
@discription: insert data into cl
@author: wangkexin
@parameter
cl: the collection to be inserted
rownums: number of set data to be inserted
***************************************************************************** */
function insertData ( cl, rownums )
{
   var record = [];
   for( var i = 0; i < rownums; i++ )
   {
      record.push( { a: i, b: i, c: "sequoiadb alter test" } );
   }
   cl.insert( record );
}

/* *****************************************************************************
@discription: check split result from source group and target group
@author: wangkexin
@parameter
csName: the split collection space
clName: the split collection
srcGroupName: source group name
tarGroupName: target group name
expDataNum: Total number of data expected to be returned
***************************************************************************** */
function checkSplitResult ( csName, clName, srcGroupName, tarGroupName, expDataNum )
{
   var actDataNum = 0;

   var dataNode1 = new Sdb( db.getRG( srcGroupName ).getMaster() );
   var checkCL1 = dataNode1.getCS( csName ).getCL( clName );
   var recordNum1 = checkCL1.count();
   actDataNum = actDataNum + recordNum1;
   dataNode1.close();

   var dataNode2 = new Sdb( db.getRG( tarGroupName ).getMaster() );
   var checkCL2 = dataNode2.getCS( csName ).getCL( clName );
   var recordNum2 = checkCL2.count();

   if( recordNum2 == 0 )
   {
      throw "The number returned by target group is 0. srcRG :" + srcGroupName + ", tarRG :" + tarGroupName;
   }
   actDataNum = actDataNum + recordNum2;
   dataNode2.close();

   if( actDataNum !== expDataNum )
   {
      throw buildException( "checkData", "check field", "total num is wrong", expDataNum, actDataNum );
   }
}

/* *****************************************************************************
@discription: check not split result from source group and target group
@author: wangkexin
@parameter
csName: the split collection space
clName: the split collection
srcGroupName: source group name
tarGroupName: target group name
expDataNum: Total number of data expected to be returned
***************************************************************************** */
function checkNotSplitResult ( csName, clName, srcGroupName, tarGroupName, expDataNum )
{
   var actDataNum = 0;

   var dataNode = new Sdb( db.getRG( srcGroupName ).getMaster() );
   var checkCL = dataNode.getCS( csName ).getCL( clName );
   actDataNum = actDataNum + checkCL.count();
   if( actDataNum !== expDataNum )
   {
      throw buildException( "checkData", "check field", "total num is wrong", expDataNum, actDataNum );
   }
   dataNode.close();

   var dataNode2 = new Sdb( db.getRG( tarGroupName ).getMaster() );
   try
   {
      //if not split, get collection from target group will fail.
      dataNode2.getCS( csName ).getCL( clName );
      throw "exp fail but found success."
   }
   catch( e )
   {
      if( e !== -23 )
      {
         throw "unexpected error: " + e;
      }
   }
}

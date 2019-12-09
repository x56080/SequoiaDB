/*******************************************************************************
*@Description : split_sync testcase common functions
*@Modify list :
*              2019-5-30 wangkexin
*******************************************************************************/

/* ****************************************************
@description: insert data into collection
@parameter: 
    cl : the collection ready to insert records
    insertNum : the number of records inserted
@return: 
**************************************************** */
function insertData ( cl, insertNum )
{
   var dataArray = new Array();
   for( var i = 0; i < insertNum; i++ )
   {
      var data = { a: i };
      dataArray.push( data );
   }
   cl.insert( dataArray );
}

/* ****************************************************
@description: create one data group with one node
@parameter: 
    rgName : the name of dataRG
    hostName : host name of new node
@return: the path of backup log
**************************************************** */
function createDataGroup ( rgName, hostName )
{
   var dataRG = db.createRG( rgName );
   var srcLogPath = "";

   var port = parseInt( RSRVPORTBEGIN ) + 10;
   var dataPath = RSRVNODEDIR + "data/" + port;
   var checkSucc = false;
   var times = 0;
   var maxRetryTimes = 10;
   do
   {
      try
      {
         dataRG.createNode( hostName, port, dataPath, { diaglevel: 5 } );
         checkSucc = true;
      }
      catch( e )
      {
         //-145 :SDBCM_NODE_EXISTED  -290:SDB_DIR_NOT_EMPTY
         if( e == -145 || e == -290 )
         {
            port = port + 10;
            dataPath = RSRVNODEDIR + "data/" + port;
         }
         else
         {
            throw "create node failed!  port = " + port + " dataPath = " + dataPath + " errorCode: " + e;
         }
         times++;
      }
   }
   while( !checkSucc && times < maxRetryTimes );
   dataRG.start();
   srcLogPath = hostName + ":" + CMSVCNAME + "@" + dataPath + "/diaglog/sdbdiag.log";
   return srcLogPath;
}

/* ****************************************************
@description: remove one data group
@parameter: 
    rgName : The name of data group to be removed 
@return:
**************************************************** */
function removeDataRG ( rgName )
{
   try
   {
      db.removeRG( rgName );
   }
   catch( e )
   {
      //-154 : SDB_CLS_GRP_NOT_EXIST
      if( e !== -154 )
      {
         throw buildException( "removeDataRG()", e, "remove dataRG failed.", '-154', e );
      }
   }
}

/* ****************************************************
@description: get the expected data array
@parameter: 
    startDataValue : The starting value of the data array
    endDataValue : value at the end of the data array
@return: the expected data array
**************************************************** */
function getExpDataArr ( startDataValue, endDataValue )
{
   var array = new Array();
   for( var i = startDataValue; i < endDataValue; i++ )
   {
      var data = { a: i };
      array.push( data );
   }
   return array;
}

/* ****************************************************
@description: check the data on the specified group
@parameter: 
    csName : the collection space name
    clName : the collection name
    groupName : the specified group name
    expArray : the expected data array
@return:
**************************************************** */
function checkData ( csName, clName, groupName, expArray )
{
   var dataNode = new Sdb( db.getRG( groupName ).getMaster() );
   var checkCL = dataNode.getCS( csName ).getCL( clName );
   var cursor = checkCL.find( {}, { "_id": { "$include": 0 } } ).sort( { a: 1 } );
   var rcRecs = new Array();
   while( tmpRecs = cursor.next() )
   {
      rcRecs.push( tmpRecs.toObj() );
   }

   var expRecs = JSON.stringify( expArray );
   var actRecs = JSON.stringify( rcRecs );
   if( expRecs !== actRecs )
   {
      throw buildException( "checkResult", null, "", expRecs, "  " + actRecs );
   }
   dataNode.close();
}

/* ****************************************************
@description: check the split result by check data count from different data node.
@parameter: 
    cl : the collection from coord data
    csName : the collection space name
    clName : the collection name
    srcGroupName : the source group name
    tarGroupName : the target group name
    expCount : the expected data count
    expSrcCount : the expected data count from srcGroup
    expTarCount : the expected data count from tarGroup
@return:
**************************************************** */
function checkSplitResultByCount ( cl, csName, clName, srcGroupName, tarGroupName, expCount, expSrcCount, expTarCount )
{
   //��coord�Ƚϼ�¼��
   var actCount = cl.count();
   if( expCount !== Number( actCount ) )
   {
      throw buildException( "checkSplitResultByCount", null, "", expCount, "  " + Number( actCount ) );
   }

   //ֱ��Դ��Ŀ��ڵ�Ƚϼ�¼��
   var srcDataNode = new Sdb( db.getRG( srcGroupName ).getMaster() );
   var checkCL1 = srcDataNode.getCS( csName ).getCL( clName );
   var actSrcCount = checkCL1.count();

   if( expSrcCount !== Number( actSrcCount ) )
   {
      throw buildException( "checkSplitResultByCount", null, "source group data count is wrong", expSrcCount, "  " + Number( actSrcCount ) );
   }
   srcDataNode.close();

   var tarDataNode = new Sdb( db.getRG( tarGroupName ).getMaster() );
   var checkCL2 = tarDataNode.getCS( csName ).getCL( clName );
   var actTarCount = checkCL2.count();

   if( expTarCount !== Number( actTarCount ) )
   {
      throw buildException( "checkSplitResultByCount", null, "target group data count is wrong", expTarCount, "  " + Number( actTarCount ) );
   }
   tarDataNode.close();
}
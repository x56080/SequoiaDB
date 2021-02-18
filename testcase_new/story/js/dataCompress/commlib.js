import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

/* ****************************************************
@description: get name of dataRG
@parameter: 
@return: 
         groupNameArray
**************************************************** */
function getDataGroupsName ()
{
   var tmpArray = commGetGroups( db );
   var groupNameArray = new Array;
   for( i = 0; i < tmpArray.length; i++ )
   {
      groupNameArray.push( tmpArray[i][0].GroupName );
   }
   return groupNameArray;
}

/* ****************************************************
@description: createCL
@parameter:
            compressed: true/false
            compreType: "lzw"/"snappy"
@return: 
      cl, eg: "localhost:11810.cs.cl"
@Msg: 设置集合ReplSize为0是因为用例中有检查所有节点数据，需要保持主备节点一致
**************************************************** */
function createCL ( csName, clName, rgName, compressed, compreType )
{

   if( compressed == false )
   {
      var options = { Group: rgName, ReplSize: 0, Compressed: false };
   }
   else if( compressed == true )
   {
      var options = { Group: rgName, ReplSize: 0, Compressed: true, CompressionType: compreType };
   }

   var cl = commCreateCL( db, csName, clName, options, true, true );

   return cl;
}

/* ****************************************************
@description: check attribute of CL
@parameter:
            csName: name of CS
            clName: name of CL
            compressed: true or false
            compreType: "lzw" or "snappy"
@return: 
**************************************************** */
function checkAttributeOfCL ( csName, clName, compressed, compreType )
{

   //check attribute of cl
   var clInfo = db.snapshot( 8, { Name: csName + "." + clName } ).current().toObj();

   var Attribute = clInfo["Attribute"];
   var AttributeDesc = clInfo["AttributeDesc"];
   var CompressionType = clInfo["CompressionType"];
   var CompressionTypeDesc = clInfo["CompressionTypeDesc"];

   if( compressed == false )
   {
      if( Attribute !== 0 || AttributeDesc !== "" )
      {
         throw new Error( "Failed to check attribute of cl fail,[checkResult]" +
            '[Attribute: 0, AttributeDesc: ""]' +
            '[Attribute: ' + Attribute + ', AttributeDesc: "' + AttributeDesc + '"]' );
      }
   }
   else if( compressed == true )
   {
      if( compreType == "snappy" )
      {
         var tmpCmprt = 0;
      }
      else if( compreType == "lzw" )
      {
         var tmpCmprt = 1;
      }
      if( Attribute !== 1 || AttributeDesc !== "Compressed"
         || CompressionType !== tmpCmprt || CompressionTypeDesc !== compreType )
      {
         throw new Error( "Failed to check attribute of cl fail,[checkResult]" +
            '[Attribute: 1, AttributeDesc: "Compressed", CompressionType: ' + tmpCmprt
            + ', CompressionTypeDesc: "' + compreType + '"]' +
            '[Attribute: ' + Attribute + ', AttributeDesc: "' + AttributeDesc
            + '", CompressionType: ' + CompressionType + ', CompressionTypeDesc: "' + CompressionTypeDesc + '"]' );
      }
   }
}

/* ****************************************************
@description: check count of records for each node
@parameter:
            csName: name of CS
            clName: name of CL
            rgName: data RG
            insertRecsNum: number of records
@return: 
**************************************************** */
function checkNodeCnt ( csName, clName, rgName, insertRecsNum )
{

   var i = 0;
   var rc = db.exec( "select NodeName from $SNAPSHOT_SYSTEM where GroupName='" + rgName + "'" );
   while( rc.next() )
   {
      i++;
      var nodeName = rc.current().toObj()["NodeName"];

      var nodeDB = new Sdb( nodeName );
      var recsCnt = nodeDB.getCS( csName ).getCL( clName ).count();

      assert.equal( recsCnt, insertRecsNum );
   }
}

/* ****************************************************
@description: check compressed rate of CL
@parameter:
            name of CS
@return: 
**************************************************** */
function checkCompressedRate ( noCSName, lzwCSName, expectRate )
{

   if( expectRate == null ) { expectRate = 1.0 };

   var noCSInfo = db.snapshot( 5, { Name: noCSName } ).current().toObj();
   var lzwCSInfo = db.snapshot( 5, { Name: lzwCSName } ).current().toObj();
   var noUsedSize = noCSInfo["TotalSize"] - noCSInfo["FreeSize"];
   var lzwUsedSize = lzwCSInfo["TotalSize"] - lzwCSInfo["FreeSize"];
   //Unit of TotalSize/FreeSize: B

   // noUsedSize:lzwUsedSize >= 1:0.8
   var compRate = lzwUsedSize / noUsedSize;
   if( compRate >= expectRate )
   {
      throw new Error( "Failed to check compressed rate. fail,[checkCompressedRate]" + "compRate <= " + expectRate + ", " + "compRate = " + compRate );
   }
}

/* ****************************************************
@description: 用例结束时删除集合空间有可能遇到压缩线程在检查数据，
              会导致删除集合空间报-147，重试删除
@parameter:
            name of CS
@author:    luweikang 
**************************************************** */
function clearCS ( db, csName )
{
   var times = 0;
   do
   {
      try
      {
         db.dropCS( csName );
         break;
      }
      catch( e )
      {
         if( e.message == SDB_LOCK_FAILED && times < 60 )
         {
            times++;
            sleep( 1000 );
         }
         else
         {
            throw e;
         }
      }
   }
   while( true )
}

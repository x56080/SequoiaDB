/*******************************************************************************
*@Description : Define public functions and variables of data compression
*@Modify list :
                  2016/3/23   XiaoNi Huang init
*******************************************************************************/

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
**************************************************** */
function createCL ( csName, clName, rgName, compressed, compreType )
{
   if( compressed == false )
   {
      println( '\n---Begin to create CL[Group:"' + rgName + '", Compressed:' + compressed + '].' );

      var options = { Group: rgName, ReplSize: 0, Compressed: false };
   }
   else if( compressed == true )
   {
      println( '\n---Begin to create CL[Group:"' + rgName + '", Compressed:' + compressed
         + ', CompressionType:"' + compreType + '"].' );

      var options = { Group: rgName, ReplSize: 0, Compressed: true, CompressionType: compreType };
   }

   var cl = commCreateCLByOption( db, csName, clName, options, true,
      true, "Failed to create CL[" + csName + "," + clName + "]." );

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
   println( "\n---Begin to check attribute of CL." );

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
         throw buildException( "Failed to check attribute of cl", null, "[checkResult]",
            '[Attribute: 0, AttributeDesc: ""]',
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
         throw buildException( "Failed to check attribute of cl", null, "[checkResult]",
            '[Attribute: 1, AttributeDesc: "Compressed", CompressionType: ' + tmpCmprt
            + ', CompressionTypeDesc: "' + compreType + '"]',
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
   println( '\n---Begin to check records for each node in the "' + rgName + '".' )

   var i = 0;
   var rc = db.exec( "select NodeName from $SNAPSHOT_SYSTEM where GroupName='" + rgName + "'" );
   while( rc.next() )
   {
      i++;
      var nodeName = rc.current().toObj()["NodeName"];

      var nodeDB = new Sdb( nodeName );
      var recsCnt = nodeDB.getCS( csName ).getCL( clName ).count();
      println( "   node: " + String( nodeDB ) + ", recsCnt: " + Number( recsCnt ) );

      if( Number( recsCnt ) !== insertRecsNum )
      {
         throw buildException( "Failed to check Records.", null, "[checkRecords]",
            "recsCnt: " + insertRecsNum, "recsCnt: " + recsCnt );
      }
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
   println( "\n---Begin to check compressed rate." );

   if( expectRate = undefined ) { expectRate == 1.0 };

   var noCSInfo = db.snapshot( 5, { Name: noCSName } ).current().toObj();
   var lzwCSInfo = db.snapshot( 5, { Name: lzwCSName } ).current().toObj();
   var noUsedSize = noCSInfo["TotalSize"] - noCSInfo["FreeSize"];
   var lzwUsedSize = lzwCSInfo["TotalSize"] - lzwCSInfo["FreeSize"];
   //Unit of TotalSize/FreeSize: B
   println( "   CS[" + noCSName + "]  UsedSize(MB):  " + noUsedSize / 1024 / 1024 );
   println( "   CS[" + lzwCSName + "] UsedSize(MB):  " + lzwUsedSize / 1024 / 1024 );

   // noUsedSize:lzwUsedSize >= 1:0.8
   var compRate = lzwUsedSize / noUsedSize;
   println( "   lzwUsedSize/noUsedSize = " + compRate );
   if( compRate >= expectRate )
   {
      throw buildException( "Failed to check compressed rate.", null, "[checkCompressedRate]",
         "compRate <= " + expectRate + ", ", "compRate = " + compRate );
   }
}
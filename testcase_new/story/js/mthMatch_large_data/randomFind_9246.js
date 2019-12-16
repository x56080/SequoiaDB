/************************************
*@Description: random data,random matches,find data
*@author:      zhaoyu
*@createdate:  2016.11.3
*@testlinkCase:seqDB-9246 
**************************************/
//CLName
var clName = COMMCLNAME + '_9246';

//query field name
var fieldNames = ['fieldName1', 'fieldName2', 'fieldName3', 'fieldName4', 'fieldName5', 'fieldName6'];

function main ()
{
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create main-sub cl for index scan
   var dbcl_IndexScan = commCreateCL( db, COMMCSNAME, clName );
   println( "createCL success" );

   //insert random numberical data
   insertRandomData( dbcl_IndexScan );
   println( "insert data success" );

   //create index for index scan
   createIndex( dbcl_IndexScan );
   println( "create Index success" );

   //query use random conditon and check result
   var loopNum = 20;
   queryDataAndCheck( dbcl_IndexScan, loopNum );
   println( "check result success" );
}
main()

/************************************
*@Description: unset the order for arr elements.
*@author:      zhaoyu 
*@createDate:  2016/7/19
*@parameters:               
**************************************/
function getRdmDataFromArr ( arr )
{
   //unset the arr order
   var newArr = arr.sort( function() { return Math.random() - 0.5 } );

   //convert array to object
   var obj = {};
   for( var i in newArr )
   {
      for( var k in newArr[i] )
      {
         obj[k] = newArr[i][k];
      }
   }
   return obj;
}

function arrToObj ( arr )
{
   var obj = {};
   for( var i in arr )
   {
      for( var k in arr[i] )
      {
         obj[k] = arr[i][k];
      }
   }
   return obj;
}

//create index
function createIndex ( dbcl_IndexScan )
{
   for( var i = 0; i < fieldNames.length; i++ )
   {
      var indexDef = {};
      var key = fieldNames[i];
      if( i % 2 == 0 )
      {
         indexDef[key] = 1;
      }
      else
      {
         indexDef[key] = -1;
      }
      var idxName = "fieldName" + i + "Index";

      try
      {
         dbcl_IndexScan.createIndex( idxName, indexDef, false, false, 1536 );
      }
      catch( e )
      {
         buildException( "createIndex", e,
            'createIndex( ' + idxName + ', ' + indexDef + ', false, false, 1536 )',
            0, e );
      }
   }
}


//insert random numberical data
function insertRandomData ( dbcl_IndexScan )
{
   for( var i = 0; i < 50; i++ )
   {
      var rd = new dataGenerator();
      var recs = rd.getRecords( 30000, ["int", "long", "float", "string", "bool", "date", "timestamp", "regex", "array", "null"], fieldNames );
      insertData( dbcl_IndexScan, recs );
      rd = null;
      recs.length = 0;  // release space
   }
}

//generate query condition and check result
function queryDataAndCheck ( dbcl_IndexScan, loopNum )
{
   for( var i = 0; i < loopNum; i++ )
   {
      //set all matches support index scan
      matches1 = ["$et", "$gt", "$gte", "$lt", "$lte", "$ne"];
      matches2 = ["$in", "$all"];

      var findCondition1 = getFindCondition( 1, ["int", "long", "float", "string", "date", "timestamp", "regex", "array"], matches1 );
      var findCondition2 = getFindCondition( 1, "array", matches2 );

      findCondition3 = [{ $exists: 1 }, { $isnull: 0 }];

      findCondition4 = { $mod: [2, 1] };

      findCondition5 = getRandomRegex();

      //convert array to object
      findConditionObj1 = arrToObj( findCondition1 );
      findConditionObj2 = arrToObj( findCondition2 );
      findConditionObj3 = arrToObj( findCondition3 );

      var obj1 = mergeObj( findConditionObj1, findConditionObj2 );
      var obj2 = mergeObj( obj1, findConditionObj3 );
      var obj3 = mergeObj( obj2, findCondition4 );

      //println("obj3:"+JSON.stringify(obj3));

      var findConditions = [];
      for( var j in obj3 )
      {
         var subcond = {};
         subcond[j] = obj3[j]
         findConditions.push( subcond );
      }
      findConditions.push( findCondition5 );
      //println("findConditions:"+JSON.stringify(findConditions));

      //generate random find condition
      var randomCondition = genRandomFindCondition( findConditions, fieldNames );
      while( randomCondition.length === 0 )
      {
         println( "randomCondition.length:" + randomCondition.length );
         randomCondition = genRandomFindCondition( findConditions, fieldNames );
      }

      var randomConditionObj = { $and: randomCondition };
      println( "randomCondition:" + JSON.stringify( randomConditionObj ) );

      //get index scan result
      var ixScanCursor = dbcl_IndexScan.find( randomConditionObj ).hint( { "": '' } ).sort( { _id: 1 } );

      //get table scan result
      var tbScanCursor = dbcl_IndexScan.find( randomConditionObj ).hint( { "": null } ).sort( { _id: 1 } );

      //check count
      var endTime = new Date();
      println( "checkResult startTime:" + endTime.toLocaleString() );
      if( ixScanCursor.count().toString() !== tbScanCursor.count().toString() )
      {
         throw buildException( "check count", null, "", ixScanCursor.count(), tbScanCursor.count() );
      }
      //println("ixScanCursor.count():"+ixScanCursor.count());


      //check every records every fields,tbScanRecs as compare source
      while( ixScanCursor.next() && tbScanCursor.next() )
      {
         var ixScanRec = ixScanCursor.current().toObj();
         delete ixScanRec["_id"];
         var tbScanRec = tbScanCursor.current().toObj();
         delete tbScanRec["_id"];
         if( JSON.stringify( ixScanRec ) !== JSON.stringify( tbScanRec ) )
         {
            //println("\n\nallDataRecs:" + JSON.stringify(allDataRecs));
            throw buildException( "check record", null, JSON.stringify( ixScanRec ), JSON.stringify( tbScanRec ) );
         }
      }
      var endTime = new Date();
      println( "checkResult endTime:" + endTime.toLocaleString() );
   }
}

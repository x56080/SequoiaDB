/************************************
*@Description: insert data
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function insertData ( dbcl, insertData )
{
   try
   {
      dbcl.insert( insertData );
      println( "--insert data success" );
   }
   catch( e )
   {
      throw buildException( "insertData()", e, "insert", "insert success", "insert fail" );
   }
}

/************************************
*@Description: remove data
*@author:      zhaoyu
*@createDate:  2016.10.12
**************************************/
function removeData ( dbcl )
{
   try
   {
      dbcl.remove();
      println( "--remove data success" );
   }
   catch( e )
   {
      throw buildException( "removeData()", e, "remove", "remove success", "remove fail" );
   }
}

/************************************
*@Description: find and sort data
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function sortFindData ( dbcl, findCondition1, findCondition2, sortCondition )
{
   try
   {
      var sortResult;
      if( typeof hintCondition == "undefined" )
      {
         sortResult = dbcl.find( findCondition1, findCondition2 ).sort( sortCondition );
      }
      else
      {
         sortResult = dbcl.find( findCondition1, findCondition2 ).sort( sortCondition ).hint( hintCondition );
      }
   }
   catch( e )
   {
      throw buildException( "sortFindData()", e, "find and sort data", "find and sort data success", "find and sort data fail" );
   }
   return sortResult;
}

/************************************
*@Description: find and explain
*@author:      zhaoyu
*@createDate:  2016.10.18
**************************************/
function explainFindData ( dbcl, findCondition1, findCondition2, sortCondition )
{
   try
   {
      var explainResult
      if( typeof hintCondition == "undefined" )
         explainResult = dbcl.find( findCondition1, findCondition2 ).sort( sortCondition ).explain();
      else
         explainResult = dbcl.find( findCondition1, findCondition2 ).sort( sortCondition ).hint( hintCondition ).explain();
   }
   catch( e )
   {
      throw buildException( "explainFindData()", e, "find and explain", "find and explain success", "find and explain fail" );
   }
   return explainResult;
}

/************************************
*@Description: get actual result and check it 
*@author:      zhaoyu
*@createDate:  2016.10.18
**************************************/
function checkExplainResult ( dbcl, findCondition1, findCondition2, sortCondition, expRecs )
{
   var rc = explainFindData( dbcl, findCondition1, findCondition2, sortCondition );
   println( "--begin to check explain" );
   checkExplainRec( rc, expRecs );
   println( "--end check explain" );
}


/************************************
*@Description: compare actual and expect result,
               they is not the same ,return error ,
               else return ok
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function checkExplainRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }
   //check count
   if( actRecs.length !== expRecs.length )
   {
      println( "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
      throw buildException( "check count", null, "",
         expRecs.length, actRecs.length );
   }

   //check every records every fields,expRecs as compare source
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];

      for( var f in expRec )
      {
         if( ( f == "Name" ) || ( f == "ScanType" ) || ( f == "IndexName" ) || ( f == "Query" ) || ( f == "IXBound" ) || ( f == "NeedMatch" ) )
         {
            if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
            {
               println( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'" );
               println( "\nactual value= " + JSON.stringify( actRec[f] ) + "\n\nexpect value= " + JSON.stringify( expRec[f] ) );
               throw buildException( "checkRec()", "rec ERROR" );
            }
         }
      }
   }
}

/************************************
*@Description: compare actual and expect result,
               they is not the same ,return error ,
               else return ok
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }
   //check count
   if( actRecs.length !== expRecs.length )
   {
      println( "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
      throw buildException( "check count", null, "",
         expRecs.length, actRecs.length );
   }

   //check every records every fields,expRecs as compare source
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];

      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            println( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'" );
            println( "\nactual record= " + JSON.stringify( actRec ) + "\n\nexpect record= " + JSON.stringify( expRec ) );
            println( "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
            throw buildException( "checkRec()", "rec ERROR" );
         }
      }
   }
   //check every records every fields,actRecs as compare source
   for( var j in actRecs )
   {
      var actRec = actRecs[j];
      var expRec = expRecs[j];

      for( var f in actRec )
      {
         if( f == "_id" )
         {
            continue;
         }
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            println( "\nerror occurs in " + ( parseInt( j ) + 1 ) + "th record, in field '" + f + "'" );
            println( "\nactual record= " + JSON.stringify( actRec ) + "\n\nexpect record= " + JSON.stringify( expRec ) );
            println( "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
            throw buildException( "checkRec()", "rec ERROR" );
         }
      }
   }
}

/************************************
*@Description: get actual result and check it 
*@author:      zhaoyu
*@createDate:  2015.5.20
**************************************/
function checkResult ( dbcl, findCondition, findCondition2, expRecs, sortCondition )
{
   var rc = sortFindData( dbcl, findCondition, findCondition2, sortCondition );
   println( "--begin to check the data" );
   checkRec( rc, expRecs );
   println( "--end check the data" );
}

/************************************
*@Description: check result when the expect result of find data is failed.
*@author:      zhaoyu 
*@createDate:  2016/4/28
*@parameters:               
**************************************/
function InvalidArgCheck ( dbcl, condition, condition2, expRecs )
{
   try
   {
      dbcl.find( condition, condition2 ).toArray();
      throw "need throw error";

   }
   catch( e )
   {
      if( expRecs != e )
      {
         throw buildException( "InvalidArgCheck() " + e + "\ncheckResult " + " \nExpect result: " + expRecs + " \nActual result:" + e );
      }
      else
      {
         println( "check result is ok!" );
      }
   }
}

/************************************
*@Description: get SrcGroup and TargetGroup info,the groups information
               include GroupName,HostName and svcname
*@author:      wuyan
*@createdate:  2015.10.14
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

/************************************
*@Description: get the informations of the srcGroups and targetGroups,then split cl with different options,
               only split 1 times
               return the informations of the srcGroups and targetGroups
*@author:      wuyan
*@createdate:  2015.10.14
**************************************/
function ClSplitOneTimes ( csName, clName, startCondition, endCondition )
{
   try
   {
      var targetGroupNums = 1;
      var groupsInfo = getSplitGroups( csName, clName, targetGroupNums );
      var srcGrName = groupsInfo[0].GroupName;
      var tarGrName = groupsInfo[1].GroupName;
      println( csName + "." + clName + "'s target group: " + tarGrName );
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
*@Description: get Group name and Service name
*@author：wuyan 2015/10/20
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
*@author:      wuyan
*@createdate:  2015.10.14
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
      var cataMaster = db.getCatalogRG().getMaster().toString().split( ":" );
      var catadb = new Sdb( cataMaster[0], cataMaster[1] );
      var Group = catadb.SYSCAT.SYSCOLLECTIONS.find().toArray();
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

/**********************************************************************
@Description:  generate all kinds of types data randomly
@author:       Ting YU
@usage:        var rd = new commDataGenerator();
               1.var recs = rd.getRecords( 300, "int", ['a','b'] );
               2.var recs = rd.getRecords( 3, ["int", "string"] );
               3.rd.getValue("string");
***********************************************************************/
function dataGenerator ()
{
   this.getRecords =
      function( recNum, dataTypes, fieldNames )
      {
         return getRandomRecords( recNum, dataTypes, fieldNames );
      }

   this.getValue =
      function( dataType )
      {
         return getRandomValue( dataType );
      }
}

function getRandomRecords ( recNum, dataTypes, fieldNames )
{
   if( fieldNames === undefined ) { fieldNames = getRandomFieldNames(); }
   if( dataTypes.constructor !== Array ) { dataTypes = [dataTypes]; }

   var recs = [];
   var i = 0;
   for( var i = 0; i < recNum; i++ )
   {
      // generate 1 record
      var rec = {};
      rec["No"] = i;
      for( var j in fieldNames )
      {
         // generate 1 filed
         var rdn = parseInt( Math.random() * dataTypes.length );
         if( rdn % 2 == 0 )
         {
            var filedName = fieldNames[j];
            var dataType = dataTypes[rdn];  //randomly get 1 data type
            var filedVal = getRandomValue( dataType );
         }
         if( filedVal !== undefined ) rec[filedName] = filedVal;
      }

      recs.push( rec );
   }
   return recs;
}

function getRandomFieldNames ( minNum, maxNum )
{
   if( minNum == undefined ) { minNum = 0; }
   if( maxNum == undefined ) { maxNum = 16; }

   var fieldNames = [];
   var fieldNum = getRandomInt( minNum, maxNum );

   for( var i = 0; i < fieldNum; i++ )
   {
      //get 1 field name
      var fieldName = "";
      var fieldNameLen = getRandomInt( 1, 9 );
      for( var j = 0; j < fieldNameLen; j++ )
      {
         //get 1 char
         var ascii = getRandomInt( 97, 123 ); // 'a'~'z'
         var c = String.fromCharCode( ascii );
         fieldName += c;
      }
      fieldNames.push( fieldName );
   }

   return fieldNames;
}

/**********************************************************************
@Description:  generate all kinds of types data randomly
@author:       Ting YU
@usage:        var rd = new commDataGenerator();
               1.var recs = rd.getRecords( 300, "int", ['a','b'] );
               2.var recs = rd.getRecords( 3, ["int", "string"] );
               3.rd.getValue("string");
***********************************************************************/
function getRandomValue ( dataType )
{
   var value = undefined;

   switch( dataType )
   {
      case "int":
         value = getRandomInt( -2147483648, 2147483647 );
         break;
      case "long":
         value = getRandomLong( -922337203685477600, 922337203685477600 );
         break;
      case "float":
         value = getRandomFloat( -999999, 999999 );
         break;
      case "array":
         value = getRandomArray();
         break;
      case "non-existed":
         break;
   }

   return value;
}

function getRandomInt ( min, max ) // [min, max)
{
   var range = max - min;
   var value = min + parseInt( Math.random() * range );
   return value;
}

function getRandomLong ( min, max )
{
   var value = getRandomInt( min, max );
   return NumberLong( value );
}

function getRandomFloat ( min, max )
{
   var range = max - min;
   var value = min + Math.random() * range;
   return value;
}

function getRandomArray ()
{
   var arr = [];
   var dataTypes = ["int", "long", "float"];

   var arrLen = getRandomInt( 1, 5 );
   for( var i = 0; i < arrLen; i++ )
   {
      var dataType = dataTypes[parseInt( Math.random() * dataTypes.length )];  //randomly get 1 data type 

      var elem = getRandomValue( dataType );
      arr.push( elem );
   }

   return arr;
}

function genRandomFindCondition ( matches, fieldNames )
{
   var recs = [];
   // generate 1 record     
   for( var j in fieldNames )
   {
      var filedName = fieldNames[j];
      var rec = {};
      // generate 1 filed
      var rdn = parseInt( Math.random() * matches.length );
      if( rdn % 2 === 0 )
      {
         var match = matches[rdn];
         rec[filedName] = match;
         recs.push( rec );
      }
   }
   return recs;
}

function getFindCondition ( recNum, dataTypes, matches )
{
   if( dataTypes.constructor !== Array ) { dataTypes = [dataTypes]; }

   var recs = [];
   for( var i = 0; i < recNum; i++ )
   {
      // generate 1 record
      var rec = {};
      for( var j in matches )
      {
         // generate 1 filed
         var match = matches[j];

         var dataType = dataTypes[parseInt( Math.random() * dataTypes.length )];  //randomly get 1 data type
         var filedVal = getRandomValue( dataType );

         if( filedVal !== undefined ) rec[match] = filedVal;
      }

      recs.push( rec );
   }
   return recs;
}

function mergeObj ( left, right )
{
   var obj = {}
   for( var k in left )
      obj[k] = left[k];

   for( var k in right )
      obj[k] = right[k];

   return obj;
}

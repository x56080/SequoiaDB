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
   }
   catch( e )
   {
      throw buildException( "insertData()", e, "insert", "insert success", "insert fail" );
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
   for( var i = 0; i < recNum; i++ )
   {
      // generate 1 record
      var rec = {};
      for( var j in fieldNames )
      {
         // generate 1 filed
         /*var filedName = fieldNames[j];

         var dataType = dataTypes[ parseInt( Math.random() * dataTypes.length ) ];  //randomly get 1 data type
         var filedVal =  getRandomValue( dataType );*/

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
      case "string":
         value = getRandomString( 0, 20 );
         break;
      case "OID":
         value = ObjectId();
         break;
      case "bool":
         value = getRandomBool();
         break;
      case "date":
         value = getRandomDate();
         break;
      case "timestamp":
         value = getRandomTimestamp();
         break;
      case "binary":
         value = getRandomBinary();
         break;
      case "regex":
         value = getRandomRegex();
         break;
      case "object":
         value = getRandomObject();
         break;
      case "array":
         value = getRandomArray();
         break;
      case "null":
         value = null;
         break;
      case "non-existed":
         break;
   }

   return value;
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

function getRandomString ( minLen, maxLen ) //string length value locate in [minLen, maxLen)
{
   var strLen = getRandomInt( minLen, maxLen );
   var str = "";

   for( var i = 0; i < strLen; i++ )
   {
      var ascii = getRandomInt( 48, 127 ); // '0' -- '~'
      var c = String.fromCharCode( ascii );
      str += c;
   }
   return str;
}

function getRandomBool ()
{
   var Bools = [true, false];
   var index = parseInt( Math.random() * Bools.length );
   var value = Bools[index];

   return value;
}

function getRandomDate ()
{
   var sec = getRandomInt( -2208902400, 253402128000 ); //1900-01-02 ~ 9999-12-30
   var d = new Date( sec * 1000 );
   var dateVal = d.getFullYear() + '-' + ( d.getMonth() + 1 ) + '-' + d.getDate();

   var value = { "$date": dateVal };
   return value;
}

function getRandomTimestamp ()
{
   var sec = getRandomInt( -2147397248, 2147397247 ); //1901-12-14-20.45.52 ~ 2038-01-18-03.14.07
   var d = new Date( sec * 1000 );

   var ns = getRandomInt( 0, 1000000 ).toString();
   if( ns.length < 6 )
   {
      var addZero = 6 - ns.length;
      for( var i = 0; i < addZero; i++ ) { ns = '0' + ns; }
   }

   var timeVal = d.getFullYear() + '-' + ( d.getMonth() + 1 ) + '-' + d.getDate() + '-' +
      d.getHours() + '.' + d.getMinutes() + '.' + d.getSeconds() + '.' + ns;

   var value = { "$timestamp": timeVal };
   return value;
}

function getRandomBinary ()
{
   var str = getRandomString( 1, 50 );

   var cmd = new Cmd();
   var binaryVal = cmd.run( "echo -n '" + str + "' | base64 -w 0" );

   var typeVal = getRandomInt( 0, 256 );
   typeVal = typeVal.toString();

   var value = { "$binary": binaryVal, "$type": typeVal };
   return value;
}

function getRandomRegex ()
{
   var opts = ["i", "m", "x", "s"];
   var index = parseInt( Math.random() * opts.length );
   var optVal = opts[index];

   var regexVal = getRandomString( 1, 10 );

   var value = { "$regex": regexVal, "$options": optVal };
   return value;
}

function getRandomObject ( n )
{
   var obj = {};
   var dataTypes = ["int", "long", "float", "string", "OID", "bool", "date",
      "timestamp", "binary", "regex", "object", "array", "null"];

   var fieldNames = getRandomFieldNames( 1, 5 );

   for( var i in fieldNames )
   {
      var dataType = dataTypes[parseInt( Math.random() * dataTypes.length )];  //randomly get 1 data type 

      var filedName = fieldNames[i]
      obj[filedName] = getRandomValue( dataType );
   }

   return obj;
}

function getRandomArray ()
{
   var arr = [];
   var dataTypes = ["int", "long", "float", "string", "date",
      "timestamp", "regex", "array"];

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

/************************************
*@Description: attach cl
*@author:      zhaoyu
*@createdate:  2016.11.23
**************************************/
function attachCL ( dbcl, subCLName, range )
{
   try
   {
      dbcl.attachCL( subCLName, range );
      println( "--attach cl success" );
   }
   catch( e )
   {
      throw buildException( "attachCL()", e, "attach cl", "attach cl success", "attach cl fail" );
   }
}

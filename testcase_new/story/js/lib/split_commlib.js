/******************************************************************************
*@Description : Public function for testing split.
*@author : XiaoJun Hu 2014.6.17 init
******************************************************************************/
import( "./basic_operation/sequoiadb.js" );

function insertData ( cl, insertNum )
{
   if( undefined == insertNum ) { insertNum = 1000; }

   var docs = [];
   for( var i = 0; i < insertNum; ++i )
   {
      var no = i;
      var name = "布斯" + i;
      var phone = 13700000000 + i;
      var company1 = "MI" + i;
      var company2 = "GOLDEN" + i;
      var date = "2020-01-13";
      var time = "2020-01-13-17.29.07.638000";
      var card = 6800000000 + i;
      var doc = { "no": no, "name": name, "phone": phone, "company1": company1, "company2": company2, "date": date, "time": time, "card": card }
      docs.push( doc );
   }
   cl.insert( docs );
   return docs
}

function checkResultsInGroup ( groupName, csName, clName, expRecords, sort )
{
   if( sort === undefined ) { sort = {}; }
   var mstDB = null;
   try
   {
      mstDB = db.getRG( groupName ).getMaster().connect();
      var cl = mstDB.getCS( csName ).getCL( clName );
      var cursor = cl.find( {}, { "_id": { "$include": 0 } } ).sort( sort );
      commCompareResults( cursor, expRecords );
   }
   finally
   {
      mstDB.close();
   }
}

function checkHashDistribution ( groupNames, csName, clName, expRecsNum )
{
   // get number of records for each group
   var actRecsNumArr = [];
   var actTotalRecsNum = 0;
   for( var i = 0; i < groupNames.length; i++ )
   {
      var rgDB = null;
      try
      {
         rgDB = db.getRG( groupNames[i] ).getMaster().connect();
         var cnt = Number( rgDB.getCS( csName ).getCL( clName ).count() );
         actRecsNumArr.push( cnt );
         actTotalRecsNum += cnt;
      }
      finally
      {
         rgDB.close();
      }
   }

   // check total number of records for all group
   if( expRecsNum !== actTotalRecsNum )
   {
      throw new Error( "expRecsNum = " + expRecsNum + ", actTotalRecsNum = " + actTotalRecsNum );
   }

   // check hash distribution, expect difference value is 0.3 of the total number of records
   var expRgRecsAvg = expRecsNum / groupNames.length;
   var expDiffVal = Math.floor( expRecsNum * 0.3 );
   for( var i = 0; i < groupNames.length; i++ )
   {
      var actDiffVal = Math.floor( Math.abs( expRgRecsAvg - actRecsNumArr[i] ) );
      if( actDiffVal > expDiffVal )
      {
         throw new Error( "expDiffVal = " + expDiffVal + ", actDiffVal = " + actDiffVal
            + ", totalRecsNum = " + actTotalRecsNum + ", groupNames = [" + groupNames + "], actRecsNumArr = [" + actRecsNumArr + "]" );
      }
   }
}

function getRandomPartition ()
{
   var baseNum = 2;
   var powNum = getRandomInt( 3, 20 ); // [2^3, 2^20]
   var rdmPow = Math.pow( baseNum, powNum );
   return rdmPow;
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

function getRandomArr ()
{
   var arrNestNum = getRandomInt( 1, 20 );

   // get obj, e.g: {f20:[ tmpValArr ]}
   var str = "大写ABCDEFGHIJKLMNOPQRSTUVWXYZ小写abcdefghijklmnopqrstuvwxyz数字0123456789中文";
   var tmpArrStr = [getRandomInt( -2147483648, 2147483647 ),
   '"' + str.substring( 0, getRandomInt( 0, str.length - 1 ) ) + '"',
   JSON.stringify( { "subobj": "test" } )];
   var tmpFieldName = "t" + ( arrNestNum - 1 );
   var objStr = "{" + tmpFieldName + ": [" + tmpArrStr + "]}";
   //println("objStr = " + objStr );

   // get an array of random nested layers
   var prefixStr = "";
   var suffixStr = "";
   for( var i = 0; i < arrNestNum - 1; i++ ) 
   {
      var fieldName = "t" + i;
      // e.g: "{a1: [{a2: [{a3: [..."
      var str1 = "{ " + fieldName + ": [";
      prefixStr += str1;
      // e.g: "}] }] }]..."
      var str2 = " ]}";
      suffixStr += str2;
   }
   // e.g: "{a1:[ {a2:[ {a3:[ {...} ]} ]} ]}"
   var arrVal = prefixStr + objStr + suffixStr;
   // to json
   var arrJson = JSON.parse( arrVal );
   //println("arrJson = " + JSON.stringify( arrJson ));

   var arr = [arrJson];
   return arr;
}

function getRandomObj ()
{
   var arrNestNum = getRandomInt( 1, 20 );

   // get obj, e.g: {f20:[ tmpValArr ]}
   var str = "大写ABCDEFGHIJKLMNOPQRSTUVWXYZ小写abcdefghijklmnopqrstuvwxyz数字0123456789中文";
   var tmpArrStr = [JSON.stringify( { "int": getRandomInt( -2147483648, 2147483647 ) } ),
   JSON.stringify( { "str": str.substring( 0, getRandomInt( 0, str.length - 1 ) ) } ),
   JSON.stringify( { "bool": true } ),
   JSON.stringify( { "null": null } )];
   var tmpFieldName = "t" + ( arrNestNum - 1 );
   var tmpObjStr = "{" + tmpFieldName + ": " + tmpArrStr[getRandomInt( 0, tmpArrStr.length - 1 )] + "}";
   //println("tmpObjStr = " + tmpObjStr );

   // get an object of random nested layers
   var prefixStr = "";
   var suffixStr = "";
   for( var i = 0; i < arrNestNum - 1; i++ ) 
   {
      var fieldName = "t" + i;
      // e.g: "{a1: {a2: {a3: ..."
      var str1 = "{ " + fieldName + ": ";
      prefixStr += str1;
      // e.g: "} } }..."
      var str2 = " }";
      suffixStr += str2;
   }
   // e.g: "{a1: {a2: {a3: {...} } } }"
   var objVal = prefixStr + tmpObjStr + suffixStr;
   // to json
   var obj = JSON.parse( objVal );
   //println("obj = " + JSON.stringify( obj ));

   return obj;
}

function getRandomStdDate ()
{
   // get YY
   var YY = "" + getRandomInt( 0, 9999 ); //YY: [0000, 9999]
   while( YY.length < 4 )
   {
      YY = "0" + YY;
   }
   // get MM
   var MM = "" + getRandomInt( 1, 12 );
   while( MM.length < 2 )
   {
      MM = "0" + MM;
   }
   // get DD
   var DD = "" + getRandomInt( 1, 28 ); //february month max days: 28
   while( DD.length < 2 )
   {
      DD = "0" + DD;
   }
   var date = YY + "-" + MM + "-" + DD;
   return date;
}

function getRandomStdTime ()
{
   // get YY
   var YY = "" + getRandomInt( 2000, 2037 );
   // get MM
   var MM = "" + getRandomInt( 1, 12 );
   while( MM.length < 2 )
   {
      MM = "0" + MM;
   }
   // get DD
   var DD = "" + getRandomInt( 1, 28 ); //february month max days: 28
   while( DD.length < 2 )
   {
      DD = "0" + DD;
   }
   // get HH 
   var HH = "" + getRandomInt( 0, 23 );
   while( HH.length < 2 )
   {
      HH = "0" + HH;
   }
   // get mm
   var mm = "" + getRandomInt( 0, 59 );
   while( mm.length < 2 )
   {
      mm = "0" + mm;
   }
   // get ss
   var ss = "" + getRandomInt( 0, 59 );
   while( ss.length < 2 )
   {
      ss = "0" + ss;
   }
   // get ffffff
   var ffffff = "" + getRandomInt( 0, 999999 ); //ffffff: [000000, 999999]
   while( ffffff.length < 6 )
   {
      ffffff = "0" + ffffff;
   }

   var date = YY + "-" + MM + "-" + DD + "-" + HH + "." + mm + "." + ss + "." + ffffff;
   return date;
}

/* ****************************************************
@description: remove one data group
@parameter:
    rgName : The name of data group to be removed
@return:
**************************************************** */
function removeRG ( rgName )
{
   try
   {
      db.removeRG( rgName );
   }
   catch( e )
   {
      if( e.message !== "-154" )
      {
         throw e;
      }
   }
}
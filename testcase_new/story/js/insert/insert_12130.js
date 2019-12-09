/******************************************************************************
*@Description : seqDB-12130:����string��array���ͼ�¼����¼���Ƚϴ�                    
*@Author      : 2019-5-29  wuyan modify
******************************************************************************/
main();
function main ()
{
   var clName = "insert12130";
   var cl = readyCL( clName );

   var expRecords = insertRecords( cl );
   var actRecords = cl.find();
   checkRec( actRecords, expRecords );

   cleanCL( clName );
}

function getRandomString ( len ) 
{
   var strLen = parseInt( Math.random() * len );
   var str = "";
   var chars = "ABCDEFGHIJKLMNOPQRATUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*";
   var maxPos = chars.length;
   for( var i = 0; i < strLen; i++ )
   {
      str += chars.charAt( Math.floor( Math.random() * maxPos ) );
   }
   return str;
}

function insertRecords ( cl )
{
   println( "---begin to insert records." );
   var strLength = 3000;
   var str1 = getRandomString( strLength );

   var str2 = "{name:\"qiu\",balance:1.2}";
   for( var i = 0; i < 1000; ++i )
   {
      str2 = str2 + ",{name:\"qiu\",balance:1.2}";
   }

   var objs = { "str1": str1, "array": [str2] };
   var doc = [];
   doc.push( objs );
   cl.insert( doc );

   return doc;
}





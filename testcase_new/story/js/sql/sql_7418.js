/******************************************************************************
@Description seqDB-7418:insert into
                        1. 字段名包含无效字符，error:-195
                        2. 字段值包含特殊字符，成功
@author liyuanyue
@date 2020-4-8
******************************************************************************/
testConf.clName = COMMCLNAME + "_7418";

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_7418";

   var invChars = ["=", ">", "<", "*", ";", ",", "\""];
   for( var i = 0; i < invChars.length; i++ )
   {
      var age = invChars[i] + "age";
      try
      {
         var sql = "insert into " + csName + "." + clName + "(" + age + ")" + " values(25)";
         db.execUpdate( sql );
         throw "insert into success when '" + invChars[i] + "' at the beginning of field,Expect errorno:-195";
      }
      catch( e )
      {
         if( !commCompareErrorCode( e, -195 ) )
         {
            throw new Error( e + " exec:" + sql + " error " );
         }
      }
   }

   var speChars = [";", ":", "{", "}", "[", "]", ",", "<", ">", "?", "/", "|", "\\", "+", "=", "-", "_", "~", "`", "!", "@", "#", "$", "%", "^", "&", "*"];
   for( var i = 0; i < speChars.length; i++ )
   {
      var value = speChars[i] + "age";
      db.execUpdate( "insert into " + csName + "." + clName + "(name) values" + "('" + value + "')" );
   }

   var rc = db.exec( "select * from " + csName + "." + clName );
   var expRecs = [
      { "name": ";age" }, { "name": ":age" }, { "name": "{age" }, { "name": "}age" },
      { "name": "[age" }, { "name": "]age" }, { "name": ",age" }, { "name": "<age" },
      { "name": ">age" }, { "name": "?age" }, { "name": "/age" }, { "name": "|age" },
      { "name": "\\age" }, { "name": "+age" }, { "name": "=age" }, { "name": "-age" },
      { "name": "_age" }, { "name": "~age" }, { "name": "`age" }, { "name": "!age" },
      { "name": "@age" }, { "name": "#age" }, { "name": "$age" }, { "name": "%age" },
      { "name": "^age" }, { "name": "&age" }, { "name": "*age" }];
   commCompareResults( rc, expRecs );
}
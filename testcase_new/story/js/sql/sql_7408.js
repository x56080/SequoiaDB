/******************************************************************************
@Description seqDB-7408:create/drop collectionspace/collection/index，name参数校验
                        1. drop/create collectionspace/collection/index
                           1. create/drop,name为空，error:create/drop cs:-195,create/drop cl:-6,
                           2. create/drop,name包含特殊非法字符，如"$"、"."、"a.",error:-6,drop cs还会有-34
                           3. create/drop,name长度128B，error:create cs/cl:-6,drop cl:-23,drop cs:-34
                           4. create/drop,name长度127B，成功 
                           5. create，name已存在，error:create cs:-33,create cl:-22
                           5. drop，name不存在，error:drop cl:-23,drop cs:-34
                        2. drop/create index
                           1. create/drop,name为空，error:-195
                           2. create/drop,name包含特殊非法字符，如"$"、"."、"a."，error:-6(create)、-47(drop) 
                           3. create/drop,name长度1024B，error：-6(create)、-47(drop)
                           4. create/drop,name长度1023B，成功
                           5. create，name已存在，error:-247
                           5. drop，name不存在，error:-47
                           6. create，索引不包含字段，error:-195
@author liyuanyue
@date 2020-4-7
******************************************************************************/
main( test )

function test ()
{
   var csName = COMMCSNAME + "_7408";
   var clName = COMMCLNAME + "_7408";
   var idxName = CHANGEDPREFIX + "_7408_idx";
   var speChars = ["$", ".", "a."];

   // create collectionspace
   var sql = "create collectionspace " + " ";
   compareError( sql, "create collectionspace success when name = ' ', Except errorno: -195", -195 );

   for( var i = 0; i < speChars.length; i++ )
   {
      var tmpcsName = speChars[i] + csName;
      var sql = "create collectionspace " + tmpcsName;
      compareError( sql, "create collectionspace success when '" + speChars[i] + "' at the beginning of the name, Except errorno: -6", -6 );
   }

   var tmpcsName = csName;
   for( var i = 0; i < 128 - csName.length; i++ )
   {
      tmpcsName += "a";
   }
   var sql = "create collectionspace " + tmpcsName;
   compareError( sql, "create collectionspace success when the length of the name is 128B (invalid length), Except errorno: -6", -6 );

   var succsName = tmpcsName.substring( 0, 127 );
   commDropCS( db, succsName );
   var sql = "create collectionspace " + succsName;
   db.execUpdate( sql );

   compareError( sql, "create collectionspace success when cs existed, Except errorno: -33", - 33 );

   // create collection
   var sql = "create collection " + succsName + "." + " ";
   compareError( sql, "create collection success when name = ' ', Except errorno: -6", -6 );

   for( var i = 0; i < speChars.length; i++ )
   {
      var tmpclName = speChars[i] + clName;
      var sql = "create collection " + succsName + "." + tmpclName;
      compareError( sql, "create collection success when '" + speChars[i] + "' at the beginning of the name, Except errorno: -6", -6 );
   }

   var tmpclName = clName;
   for( var i = 0; i < 128 - clName.length; i++ )
   {
      tmpclName += "a";
   }
   var sql = "create collection " + succsName + "." + tmpclName;
   compareError( sql, "create collection success when the length of the name is 128B (invalid length), Except errorno: -6", -6 );

   var succlName = tmpclName.substring( 0, 127 );
   commDropCL( db, succsName, succlName );
   var sql = "create collection " + succsName + "." + succlName;
   db.execUpdate( sql );

   compareError( sql, "create collection success when cl existed, Except errorno: -22", -22 );

   // create index
   var sql = "create index " + " " + " on " + succsName + "." + succlName + " (name)";
   compareError( sql, "create index success when name = ' ', Except errorno: -195", -195 );

   for( var i = 0; i < speChars.length; i++ )
   {
      var tmpidxName = speChars[i] + idxName;
      var sql = "create index " + tmpidxName + " on " + succsName + "." + succlName + " (name)";
      compareError( sql, "create index success when '" + speChars[i] + "' at the beginning of the name, Except errorno: -6", -6 );
   }

   var tmpidxName = idxName;
   for( var i = 0; i < 1024 - idxName.length; i++ )
   {
      tmpidxName += "a";
   }
   var sql = "create index " + tmpidxName + " on " + succsName + "." + succlName + " (name)";
   compareError( sql, "create index success when the length of the name is 1024B (invalid length), Except errorno: -6", -6 );

   var sucidxName = tmpidxName.substring( 0, 1023 );
   var sql = "create index " + sucidxName + " on " + succsName + "." + succlName + " (name)";
   db.execUpdate( sql );

   compareError( sql, "create index success when index existed, Except errorno: -247", -247 );

   var sql = "create index " + idxName + " on " + succsName + "." + succlName + " ";
   compareError( sql, "create index success when field empty,Except errorno:-195", -195 );

   // drop index
   var sql = "drop index " + " " + " on " + succsName + "." + succlName;
   compareError( sql, "drop index success when name = ' ', Except errorno: -195", -195 );

   for( var i = 0; i < speChars.length; i++ )
   {
      var tmpidxName = speChars[i] + idxName;
      var sql = "drop index " + tmpidxName + " on " + succsName + "." + succlName;
      compareError( sql, "drop index success when '" + speChars[i] + "' at the beginning of the name, Except errorno: -47", -47 );
   }

   var tmpidxName = sucidxName + "a";
   var sql = "drop index " + tmpidxName + " on " + succsName + "." + succlName;
   compareError( sql, "drop index success when the length of the name is 1024B (invalid length), Except errorno: -47", -47 );

   var sql = "drop index " + sucidxName + " on " + succsName + "." + succlName;
   db.execUpdate( sql );

   compareError( sql, "drop index success when index not exist, Except errorno: -47", -47 );

   // drop collection
   var sql = "drop collection " + succsName + "." + " ";
   compareError( sql, "drop collection success when name = ' ', Except errorno: -6", -6 );

   for( var i = 0; i < speChars.length; i++ )
   {
      var tmpclName = speChars[i] + clName;
      var sql = "drop collection " + succsName + "." + tmpclName;
      compareError( sql, "drop collection success when '" + speChars[i] + "' at the beginning of the name, Except errorno: -6", -6 );
   }

   var tmpclName = succlName + "a";
   var sql = "drop collection " + succsName + "." + tmpclName;
   compareError( sql, "drop collection success when the length of the name is 128B (invalid length), Except errorno: -23", -23 );
   var sql = "drop collection " + succsName + "." + succlName;
   db.execUpdate( sql );

   compareError( sql, "drop collection success when cl not exist, Except errorno: -23", -23 );

   // drop collectionspace
   var sql = "drop collectionspace " + " ";
   compareError( sql, "drop collectionspace success when name = ' ', Except errorno: -195", -195 );

   for( var i = 0; i < speChars.length; i++ )
   {
      var tmpcsName = speChars[i] + csName;
      var sql = "drop collectionspace " + tmpcsName;
      compareError( sql, "drop collectionspace success when '" + speChars[i] + "' at the beginning of the name, Except errorno: -6 or -34", -6, -34 );
   }

   var tmpcsName = succsName + "a";
   var sql = "drop collectionspace " + tmpcsName;
   compareError( sql, "drop collectionspace success when the length of the name is 128B (invalid length), Except errorno: -34", -34 );

   var sql = "drop collectionspace " + succsName;
   db.execUpdate( sql );

   compareError( sql, "drop collectionspace success when cs not exist, Except errorno: -34", -34 );
}
function compareError ( sql, message, code, otherCode )
{
   try
   {
      db.execUpdate( sql );
      throw message;
   }
   catch( e )
   {
      if( !commCompareErrorCode( e, code ) && ( otherCode === undefined || !commCompareErrorCode( e, otherCode ) ) )
      {
         throw new Error( e + " exec:" + sql + " error " );
      }
   }
}
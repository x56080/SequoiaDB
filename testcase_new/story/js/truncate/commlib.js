/******************************************************************************
*@Description: testcases for normal table
*@Modify list:
*              2015-5-8  xiaojun Hu   Init
******************************************************************************/
function checkResult ( real, expect )
{
   if( typeof ( real ) !== "object" && typeof ( expect ) !== "object" )
   {
      return false;
   }

   for( var key in expect )
   {
      if( undefined === typeof ( real[key] ) )
      {
         return false;
      }

      if( real[key] != expect[key] )
      {
         println( "<" + key + "> expect: " + expect[key] + ", actual: " );
         return false;
      }
   }

   if( undefined !== typeof ( real["Indexes"] ) &&
      undefined !== typeof ( real["TotalIndexPages"] ) )
   {
      return real.Indexes * 2 === real.TotalIndexPages;
   }

   return true;
}

function getCLSnapShotInfo ( db, tableName )
{
   var snapShotInfoSet = [];
   try
   {
      var snpDtl = db.snapshot( 4, { Name: tableName } ).current().toObj().Details;
      for( var i in snpDtl )
      {
         var dtlOneRg = snpDtl[i];
         if( "" === dtlOneRg.GroupName )
         {
            snapShotInfoSet.push( dtlOneRg );
            println( "exit ............" )
            break;
         }

         var rgName = dtlOneRg.GroupName;
         var materNode = db.getRG( rgName ).getMaster();

         var dtlAllNode = dtlOneRg.Group;
         for( var j in dtlAllNode )
         {
            var dtlOneNode = dtlAllNode[j];
            var nodeName = dtlOneNode.NodeName;
            if( nodeName == materNode )   // use '==' instand of '===', because type is diffent
            {
               snapShotInfoSet.push( dtlOneNode );
               break;
            }
         }
      }
   }
   catch( e )
   {
      throw buildException( "getSnapshot(4,{Name:" + tableName + "})", e );
   }
   return snapShotInfoSet;
}

/*******************************************************************************
*@Description: 对truncate的操作的验证, 使用db.snapshot(4)来验证.主要看其中的:
*              "TotalRecords"/"TotalDataPages"/"TotalIndexPages"/"TotalLobPages"
*@Parameters:
*   db: db连接
*   tableName: 表的全名, 由集合空间和集合共同组成, 如"foo.bar"
*   jsonObj: JSON对象, 如: {"TotalRecords":1, "TotalDataPages":2, ...}
*@Return: no return
*@modify:   TingYU 2016-06-06
********************************************************************************/
function truncateVerify ( db, tableName, obj )
{
   if( undefined === obj )
   {
      var obj = { TotalRecords: 0, TotalDataPages: 0, TotalLobPages: 0 };
   }
   var snapShotInfoSet = getCLSnapShotInfo( db, tableName );
   for( var i = 0; i < snapShotInfoSet.length; ++i )
   {
      var snapshotOfCLPerNode = snapShotInfoSet[i];
      if( !checkResult( snapshotOfCLPerNode, obj ) )
      {
         commPrint( snapshotOfCLPerNode );
         throw buildException( "truncateVerify", "compare error" );
      }
   }
}

/*******************************************************************************
*@Description: 向表中写入数据
*@Parameters:
*   cl: collection连接句柄
*   recordNumber: 写入的记录条数
*   recordSize: 记录的大小
*   record: 自己构造的记录，为json对象. 在写入进加入_id字段
*@Return: no return
********************************************************************************/
function truncateInsertRecord ( cl, recordNumber, recordSize, recordPass )
{
   var funcName = "truncateInsertRecord";
   if( undefined == recordNumber ) { recordNumber = 1; }
   try
   {
      if( undefined == cl ) { throw "no collection handle"; }
      var recs = [];
      for( var i = 0; i < recordNumber; ++i )
      {
         if( undefined == recordSize && undefined == recordPass )
         {
            // Integer Random
            var integerNumber = Math.random() * ( 2147483647 + 2147483648 ) - 2147483648;
            // String Random
            var strLength = 10;
            var source = "abcdefghijklmopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" +
               "`1234567890-=~!@#$%^&*()_+{}[]\\|;':\",./<>?";
            var string = "";
            for( var j = 0; j < strLength; j++ )
            {
               string += source.charAt( Math.ceil( Math.random() * 1000 ) % source.length );
            }
            // Record
            var record = {
               "id": i, "integerKey": integerNumber,
               "longIntgerKey": 72036854775807,
               "floatPointKey": 1.7E+308, "stringKey": string + i,
               "objectIDKey": { "$oid": "123abcd00ef12358902300ef" },
               "boolKey": true, "dateKey": SdbDate(),
               "timestampKey": Timestamp(),
               "binaryKey": { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" },
               "regexKey": { "$regex": "^张", "$options": "i" },
               "subObjKey": { "subobj": string },
               "arrayKey": ["abc", 0, 891, { "time": "number" }, string],
               "nullKey": null
            };
            recs.push( record );
         }
         else
         {
            var record = { "ID_Default": i };   // 17Byte
            if( undefined == recordPass )
            {
               var size = recordSize - 17;
               var source = "abcdefghijklmopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" +
                  "`1234567890-=~!@#$%^&*()_+{}[]\\|;':\",./<>?";
               var string = "";
               for( var j = 0; j < size; j++ )
               {
                  string += source.charAt( Math.ceil( Math.random() * 1000 ) % source.length );
               }
               record["description"] = string;
            }
            else
            {
               for( var recordKey in recordPass )
               {
                  record[recordKey] = recordPass[recordKey];
               }
            }
            recs.push( record );
         }
      }
      cl.insert( recs );
      if( recordNumber != cl.count() && undefined == recordPass )
      {
         throw "insert error number record";
      }
   }
   catch( e )
   {
      throw buildException( funcName, e, "cl.insert(<record>)",
         recordNumber, cl.count() )
   }
}

/*******************************************************************************
*@Description: 向表中写入LOB数据
*@Parameters:
*   cl: collection连接句柄
*   lobSize: 写入的LOB大小
*   lobNumber: 写入的LOB记录的数量
*@Return: 返回记录OID的数组.
********************************************************************************/
function truncatePutLob ( cl, lobSize, lobNumber )
{
   if( undefined == lobSize ) { lobSize = 1024; }
   if( undefined == lobNumber ) { lobNumber = 1; }
   var funcName = "truncatePutLob";
   var lobIDs = new Array();
   try
   {
      for( var n = 0; n < lobNumber; ++n )
      {
         var fileName = "tempLobFile";
         // file
         var length = lobSize || 32; // if (lobSize == 0) length = 32;
         var source = "abcdefghijklmopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ" +
            "`1234567890-=~!@#$%^&*()_+{}[]\\|;':\",./<>?";
         var string = "";
         for( var i = 0; i < length; i++ )
         {
            string += source.charAt( Math.ceil( Math.random() * 1000 ) % source.length );
         }
         var file = new File( fileName );
         file.write( string );
         var lobID = cl.putLob( fileName );
         lobIDs.push( lobID );
         File.remove( fileName );
      }
      // verify lobs
      var listLobs = cl.listLobs().toArray();
      if( lobNumber != listLobs.length )
      {
         println( listLobs );
         println( "expect lob number: " + lobNumber +
            ", actual: " + listLobs.length );
         throw "error lob numbers before truncate";
      }
      return lobIDs;
   }
   catch( e )
   {
      throw buildException( funcName, e );
   }
}



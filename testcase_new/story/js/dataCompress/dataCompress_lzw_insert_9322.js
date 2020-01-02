/************************************************************************
@Description:  seqDB-9322:插入记录包含大量重复子串，且重复子串长度>255个字节
@input:    
         1 create CL[Compressed:false] ;
           create CL[Compressed: true, CompressionType: "lzw"] ;
         2 insert:
                 random string, and substr is repeat
         3 check records, get random records, then compare the records;
           check records for each node in the group;
           check compressed rate. 
@output:   successfull
@Author:   
           2016/8/16   XiaoNi Huang init
************************************************************************/
main();

function main ()
{
   try
   {
      var noCSName = COMMCSNAME + "_no";
      var lzwCSName = COMMCSNAME + "_lzw";
      var noCLName = COMMCLNAME + "_no";
      var lzwCLName = COMMCLNAME + "_lzw";

      var rgName = getDataGroupsName()[0];
      var insertRecsNum = 300000;  //total number
      var checkRecsNum = 3;

      println( "\n---Begin to drop CS in the pre-condition." );
      commDropCS( db, noCSName, true, "Failed to drop CS[" + noCSName + "]." );
      commDropCS( db, lzwCSName, true, "Failed to drop CS[" + lzwCSName + "]." );

      println( "\n---Begin to create CS." );
      commCreateCS( db, noCSName, false, "Failed to create CS[" + noCSName + "]." );
      commCreateCS( db, lzwCSName, false, "Failed to create CS[" + lzwCSName + "]." );
      var noCL = createCL( noCSName, noCLName, rgName, false );
      var lzwCL = createCL( lzwCSName, lzwCLName, rgName, true, "lzw" );

      //get random data
      var str1 = getRandomStr1();
      var str2 = getRandomStr2();
      var str3 = getRandomStr3();
      insertRecs( noCL, noCLName, insertRecsNum, str1, str2, str3 );
      insertRecs( lzwCL, lzwCLName, insertRecsNum, str1, str2, str3 );

      checkRecs( noCL, lzwCL, insertRecsNum, checkRecsNum );
      checkNodeCnt( lzwCSName, lzwCLName, rgName, insertRecsNum );
      checkCompressedRate( noCSName, lzwCSName );

      println( "\n---Begin to drop cs in the end-condition." );
      commDropCS( db, noCSName, false, "Failed to drop CS[" + noCSName + "]." );
      commDropCS( db, lzwCSName, false, "Failed to drop CS[" + lzwCSName + "]." );
   }
   catch( e )
   {
      throw e;
   }
}

function getRandomStr1 () 
{
   println( "\n---Begin to get random string[substr is int]." );
   //str e.g: "000000000000.1111111111....."

   var data = ["0", "1", "2", "3", "4", "0", "5", "6", "7", "8", "9", "0"];

   var str1 = "";
   var tmpC1 = Math.floor( Math.random() * data.length );
   var str1 = "";
   for( var i = 0; i < 300; i++ )
   {
      str1 += data[tmpC1];
   }

   var tmpC2 = Math.floor( Math.random() * data.length );
   var str2 = "";
   for( var i = 0; i < 50; i++ )
   {
      str2 += data[tmpC2];
   }

   var str = str1 + "." + str2;
   println( "str: \n" + str );
   return str;
}

function getRandomStr2 () 
{
   println( "\n---Begin to get random string[substr is int&&letter]." );
   //str e.g: "12345555adbccccc......."

   var data = ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "a", "b", "c"];
   var str = "";

   for( var i = 0; i < 300; i++ )
   {
      var c = Math.floor( Math.random() * data.length );
      str += data[c];
   }

   println( "str: \n" + str );
   return str;
}

function getRandomStr3 () 
{
   println( "\n---Begin to get random string[substr is int&&ascii]." );
   //str e.g: "123455$%$%455adbccccc....."

   var strLen = getRandomInt( 254, 300 );
   var str = "";

   for( var i = 0; i < strLen; i++ )
   {
      var ascii = getRandomInt( 48, 127 ); // '0' -- '~'
      var c = String.fromCharCode( ascii );
      str += c;
   }

   println( "str: \n" + str );
   return str;
}

function getRandomInt ( min, max )
{
   var range = max - min;
   var value = min + parseInt( Math.random() * range );
   return value;
}

function insertRecs ( cl, clName, insertRecsNum, str1, str2, str3 )
{
   println( "\n---Begin to insert records for cl[" + clName + "]." );

   for( k = 0; k < insertRecsNum; k += 50000 )
   {
      var doc = [];
      for( i = 0 + k; i < 50000 + k; i++ )
      {
         doc.push( { "atest": str1 + i, "btest": str2 + i, "ctest": str3 + i, "num": i } )
      };
      cl.insert( doc )
   };
}

function checkRecs ( noCL, lzwCL, insertRecsNum, checkRecsNum )
{
   println( "\n---Begin to check Records." );

   for( j = 0; j < checkRecsNum; j++ )
   {
      var i = parseInt( Math.random() * insertRecsNum );
      println( "   random i: " + i );

      var noFindRc = noCL.find( { "num": i }, { _id: { $include: 0 } } ).current().toObj();
      var noRecs = JSON.stringify( noFindRc );
      //println("   noRecs: \n"+ noRecs +"\n" );

      var lzwFindRc = lzwCL.find( { "num": i }, { _id: { $include: 0 } } ).current().toObj();
      var lzwRecs = JSON.stringify( lzwFindRc );
      //println("   lzwRecs: \n"+ lzwRecs +"\n" );

      if( noRecs !== lzwRecs )
      {
         throw buildException( "Failed to check Records.", null, "[checkRecords]",
            "noRecs === lzwRecs",
            "\nnoRecs: " + noRecs + "\nlzwRecs: " + lzwRecs );
      }

   }
}
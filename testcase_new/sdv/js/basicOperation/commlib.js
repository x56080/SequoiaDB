/****************************************************
@description: commlib of basicOperation sdv
@modify list:
              2015-10-13 Ting YU init
****************************************************/
var csName = COMMCSNAME + "_basicOperation_test";
var clName = COMMCLNAME + "_basicOperation_test";

function readyCL ( option )
{
   println( "\n---begin to excute " + getFuncName() );

   commDropCL( db, csName, clName, true, true, "drop cl in begin" );
   var cl = commCreateCLByOption( db, csName, clName, option, true, false, "create cl in begin" );

   return cl;
}

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

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         if( JSON.stringify( actRec[f] ) !== JSON.stringify( expRec[f] ) )
         {
            println( "\nerror occurs in " + ( parseInt( i ) + 1 ) + "th record, in field '" + f + "'" );
            println( "\nactual recs in cl= " + JSON.stringify( actRecs ) + "\n\nexpect recs= " + JSON.stringify( expRecs ) );
            throw buildException( "checkRec()", "rec ERROR" );
         }
      }
   }
}

function checkExplain ( rc, idxName )
{
   var plan = rc.explain().current().toObj();
   if( ( plan.ScanType === "ixscan" ) && ( plan.IndexName === idxName ) )
   {	//ok
   }
   else
   {
      throw buildException( "checkExplain()", null, "query.explain()",
         "ScanType:ixscan, IndexName:" + idxName,
         "ScanType:" + plan.ScanType + ", IndexName:" + plan.IndexName );
   }
}

function create1File ( fileName, fileSize )
{
   var cmd = new Cmd();
   var str = "dd if=/dev/zero of=" + fileName + " bs=" + fileSize + " count=1";
   cmd.run( str );

   var md5 = cmd.run( "md5sum " + fileName ).split( " " )[0];
   return md5;
}

function putSomeLobs ( cl, fileName, lobNum )
{
   println( "\n---begin to excute " + getFuncName() );

   var lobIdArr = [];
   for( var i = 0; i < lobNum; i++ )
   {
      var lobId = cl.putLob( fileName );
      lobIdArr.push( lobId );
   }
   return lobIdArr;
}

function deleteSomeLobs ( cl, lobIdArr )
{
   println( "\n---begin to excute " + getFuncName() );

   for( var i in lobIdArr )
   {
      cl.deleteLob( lobIdArr[i] );
   }
}

function checkLob ( cl, expLobArr, srcMd5 )
{
   println( "\n---begin to excute " + getFuncName() );

   var rc = cl.listLobs();

   //check Available
   var lobArr = [];
   while( rc.next() )
   {
      var lobInfo = rc.current().toObj();
      var lobId = lobInfo["Oid"]["$oid"];
      var isNormal = lobInfo["Available"];
      lobArr.push( lobId );

      if( isNormal !== true )
      {
         println( "lobId=" + lobId );
         throw buildException( "check Available", null, "cl.listLobs()",
            '"Available":true', '"Available":' + isNormal );
      }
   }

   //check lob number 
   if( lobArr.length !== expLobArr.length )
   {
      throw buildException( "check lob number", null, "cl.listLobs()",
         'lob number:' + expLobArr.length,
         'lob number:' + lobArr.length );
   }

   //check lob Id
   for( var i in expLobArr )
   {
      if( lobArr[i] !== expLobArr[i] )
      {
         throw buildException( "check lob Id", null, "cl.listLobs()",
            'lob Id:' + expLobArr[i],
            'lob Id:' + lobArr[i] );
      }
   }

   //get lob and check md5sum
   var fileName = CHANGEDPREFIX + "_lobtest_getlob.file";
   for( var i in lobArr )
   {
      var lobId = lobArr[i];
      cl.getLob( lobId, fileName, true );

      var cmd = new Cmd();
      var desMd5 = cmd.run( "md5sum " + fileName ).split( " " )[0];
      if( desMd5 !== srcMd5 )
      {
         throw buildException( "get lob and check md5sum", null, "md5sum " + fileName,
            srcMd5, desMd5 );
      }

      cmd.run( "rm -rf " + fileName );
   }
}

function clean ()
{
   println( "\n---begin to excute " + getFuncName() );

   commDropCL( db, csName, clName, true, true, "drop cl in clean" );
}

function getFuncName ()
{
   var func = getFuncName.caller.toString();
   var re = /function\s*(\w*)/i;
   var funcName = re.exec( func );

   return funcName[1] + "()";
}

/************************************
*@Description:  seqDB-19300 主表上listLobs指定cond匹配条件不正确
*@author:      wuyan
*@createDate:  2019.9.5
**************************************/
try
{
   main();
}
catch( e )
{
   if( e.constructor === Error )
   {
      println( e.stack );
   }
   throw e;
}

function main ()
{
   if( commIsStandalone( db ) )
   {
      println( "skip standalone mode" );
      return;
   }

   var mainCLName = "mainCL19300";
   var subCLName = "subcl19300";
   var subCLNum = 2;
   var filePath = WORKDIR + "/subCLLob19300/";
   var scope = 5;
   var beginBound = 20190801;
   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the beginning" );
   var mainCL = createMainCLAndAttachCL( db, COMMCSNAME, mainCLName, subCLName, "YYYYMMDD", subCLNum, beginBound, scope );

   //put lob
   var lobSizes = [1024, 10, 36, 1024 * 10, 1024 * 15, 1024 * 20, 3, 1, 2, 0];
   for( var i = 0; i < lobSizes.length; ++i )
   {
      var fileName = "lob_" + lobSizes[i];
      makeTmpFile( filePath, fileName, lobSizes[i] );
      var beginDate = beginBound + i;
      insertLob( mainCL, filePath + fileName, "YYYYMMDD", scope, 1, 1, beginDate );
   }

   //listlobs with error cond
   try
   {
      println( "---Begin to listLobs with error cond.eg:cond( {} ).cond( {'Size':{'$include':0}} )" );
      mainCL.listLobs( SdbQueryOption().cond( {} ).cond( { "Size": { "$include": 0 } } ) );
      throw "listLobs should be fail!";
   }
   catch( e )
   {
      if( -6 !== e )
      {
         throw new Error( e );
      }
   }

   try
   {
      println( "---Begin to listLobs with error cond.eg:cond( {'Size':{$kk:0}} )" );
      mainCL.listLobs( SdbQueryOption().cond( { "Size": { $kk: 0 } } ) );
      throw "listLobs should be fail!";
   }
   catch( e )
   {
      if( -6 !== e )
      {
         throw new Error( e );
      }
   }

   //listLobs with correct condition
   var expSize = 1024;
   listLobsWithCondAndCheckResult( mainCL, expSize );

   //listLobs with correct selCondition
   listLobsWithSelAndCheckResult( mainCL, lobSizes );

   commDropCL( db, COMMCSNAME, mainCLName, true, true, "drop CL in the ending" );
   deleteTmpFile( filePath );
}

function listLobsWithCondAndCheckResult ( mainCL, expSize )
{
   println( "---begin to listLob with cond." );
   var listResult = mainCL.listLobs( SdbQueryOption().cond( { Size: expSize } ) );
   var actListResult = [];
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      var sizeValue = listObj["Size"];
      if( Number( sizeValue ) !== Number( expSize ) )
      {
         throw new Error( "expect Size: " + expSize + "actual Size: " + sizeValue );
      }
      actListResult.push( listObj );
   }

   var expNum = 1;
   if( Number( actListResult.length ) !== Number( expNum ) )
   {
      throw new Error( "expect listNum: " + expNum + "actual listNum: " + actListResult.length
         + "/nactListResult:" + JSON.stringify( actListResult ) );
   }
}

function listLobsWithSelAndCheckResult ( mainCL, lobSizes )
{
   println( "---begin to listLob with sel. " );
   var listResult = mainCL.listLobs( SdbQueryOption().sel( { Size: { "$include": 1 } } ).sort( { "Size": 1 } ) );
   var actListResult = [];
   while( listResult.next() )
   {
      var listObj = listResult.current().toObj();
      actListResult.push( listObj );
   }

   //lobSizes sort Ascending
   lobSizes.sort( function( a, b ) { return a - b; } );
   var expListResult = [];
   for( var i = 0; i < lobSizes.length; i++ )
   {
      var value = lobSizes[i];
      expListResult.push( { "Size": value } );
   }

   if( JSON.stringify( expListResult ) !== JSON.stringify( actListResult ) )
   {
      throw new Error( "/nexpectResult: " + JSON.stringify( expListResult ) + "/nactualResult: " + JSON.stringify( actListResult ) );
   }
}



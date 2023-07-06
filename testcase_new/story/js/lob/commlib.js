/******************************************************************************
*@Description : Test the hint index common function.
*@Modify list :
*               2014-6-12   xiaojun Hu  Init
*               2014-11-10  xiaojun Hu  Change
******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );


function lobFileIsExist ( fileName )
{
   var isExist = false;
   try
   {
      var cmd = new Cmd();
      cmd.run( "ls " + fileName );
      isExist = true;
   }
   catch( e )
   {
      if( 2 == e.message ) { isExist = false; }
   }
   return isExist;
}

function getMd5ForFile ( testFile )
{
   var cmd = new Cmd();
   var md5Arr = cmd.run( "md5sum " + testFile ).split( " " );
   var md5 = md5Arr[0];
   return md5;
}

function lobGenerateFile ( fileName, fileLine )
{
   if( undefined == fileLine )
   {
      fileLine = 1000;
   }

   var cnt = 0;
   while( true == lobFileIsExist( fileName ) )
   {
      File.remove( fileName );
      if( cnt > 10 ) break;
      cnt++;
      sleep( 10 );
   }

   if( 10 <= cnt )
      throw new Error( "failed to remove file: " + fileName );
   var file = new File( fileName );
   for( var i = 0; i < fileLine; ++i )
   {
      var record = '{ no:' + i + ', score:' + i + ', interest:["movie", "photo"], ' +
         '  major:"计算机软件与理论", dep:"计算机学院", ' +
         '  info:{name:"Holiday", age:22, sex:"男"} }';
      file.write( record );
   }
   if( false == lobFileIsExist( fileName ) )
      throw new Error( "NoFile: " + fileName );
}

function lobCreateCS ( db, DOMCSNAME, domName )
{
   var cs = db.createCS( DOMCSNAME, { "PageSize": 4096, "Domain": domName } );
   return cs;
}

function lobPutLob ( cl, lobFile, lobNum )
{
   if( undefined == lobNum )
   {
      lobNum = 10;
   }

   var oid = [];
   for( var i = 0; i < lobNum; ++i )
   {
      oid[i] = cl.putLob( lobFile );
   }
   // verify
   var cursor = cl.listLobs().toArray();
   assert.equal( lobNum, cursor.length );
   return oid;
}

// Insert data to SequoiaDB
function lobInsertDoc ( cl, recordNum )
{
   var docs = [];
   // insert 10000 records in CL
   for( var i = 0; i < recordNum; ++i )
   {
      docs.push(
         {
            no: i, score: i, interest: ["movie", "photo"],
            major: "计算机软件与理论", dep: "计算机学院",
            info: { name: "Holiday", age: 22, sex: "男" }
         } );

   }
   cl.insert( docs );
   var cnt = 0;
   do
   {
      ++cnt;
      sleep( 10 );
   }
   while( recordNum != cl.count() && 1000 < cnt );
   assert.equal( recordNum, cl.count() );
}

function lobSplit ( cl, srcGroup, dstGroup, firstCond, secondCond )
{
   if( undefined == secondCond ) { secondCond = "percent"; }
   if( "percent" != secondCond )
   {
      cl.split( srcGroup, dstGroup, firstCond, secondCond );
   }
   else
   {
      cl.split( srcGroup, dstGroup, firstCond );
   }
}

function lobGetAllGroupNames ( db )
{
   var RG = commGetGroups( db );
   var groupnames = [];
   for( var i = 0; i < RG.length; ++i )
   {
      groupnames.push( RG[i][0].GroupName );
   }
   return groupnames;
}

/******************************************************************************
 * @description: 指定文件名生成文件，并写入数据。会自动删除已经存在的老文件
 * @param {string} fileName  // 文件名
 * @param {string} data      // 数据
 ******************************************************************************/
function generateLobFile ( fileName, data )
{
   removeLobFile( fileName );

   var file = new File( fileName );
   if ( undefined != data )
   {
      file.write( data );
   }
   file.close();
}

/******************************************************************************
 * @description: 删除文件
 * @param {string} fileName  // 文件名
 ******************************************************************************/
function removeLobFile ( fileName ) {
    if ( File.exist( fileName ) ) {
        File.remove( fileName );
    }
}

 /******************************************************************************
 * @description: 调用 _putLobFile 接口上传 lob 后，并检查 lob 数据是否正确
 * @param {string} cl       // 集合名
 * @param {string} lobId    // lob id，选填
 * @param {string} dataFile // 待上传的 lob 数据文件
 * @param {string} readFile // 读取 lob 时接收数据的文件
 ******************************************************************************/
 function putAndcheckLob( cl, lobId, dataFile, readFile ) {

   if ( undefined == lobId )
   {
       lobId = cl._putLobFile( dataFile );
   }
   else
   {
       cl._putLobFile( dataFile, lobId );
   }

   cl.getLob( lobId, readFile, true );
   
   var expectMD5 = File.md5( dataFile );
   var actualMD5 = File.md5( readFile );
   removeLobFile( readFile );

   assert.equal( expectMD5, actualMD5 );
}

/******************************************************************************
 * @description: 生成指定长度的 string，可指定字符
 * @param {int} len   // 长度
 * @param {char} char // 字符
 ******************************************************************************/
function generateLobData ( len, char ) {
   if ( undefined == char ) {
       char = 'a';
   }

   var arr = new Array();
   for( var i = 0; i < len; i++ ) {
       arr[i] = char;
   }
   return arr.join('');
}
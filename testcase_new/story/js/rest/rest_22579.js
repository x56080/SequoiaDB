/************************************
*@Description: insert接口支持传入flag和options参数
*@author:      zhaohailin
**************************************/
main( test );
function test ()
{
   var clName = COMMCLNAME + "_22579";
   commDropCL( db, COMMCSNAME, clName );
   var restcl = commCreateCL( db, COMMCSNAME, clName, {}, true, false, "create cl in begin" );

   restcl.createIndex( "nameIndex", { name: 1 }, { Unique: true, NotNull: true } );

   var record = [{ 'age': 1, 'name': 'user1' }, { 'age': 2, 'name': 'user2' }, { 'age': 3, 'name': 'user3' }, { 'age': 4, 'name': 'user4' }];
   restcl.insert( record );

   //REST接口flag参数兼容SDB_INSERT_CONTONDUP,当发生索引键冲突时，跳过该记录并插入其他记录
   checkFlagType( restcl, COMMCSNAME, clName, 11, "user1", "SDB_INSERT_CONTONDUP", [{ 'age': 1, 'name': 'user1' }], 0 );
   checkFlagType( restcl, COMMCSNAME, clName, 11, "user11", "SDB_INSERT_CONTONDUP", [{ 'age': 11, 'name': 'user11' }], 0 );

   //REST接口flag参数兼容SDB_INSERT_REPLACEONDUP,当发生索引冲突时，将已存在的记录更新为待插入的新记录(替换)，并继续插入其他记录
   checkFlagType( restcl, COMMCSNAME, clName, 22, "user2", "SDB_INSERT_REPLACEONDUP", [{ 'age': 22, 'name': 'user2' }], 0 );
   checkFlagType( restcl, COMMCSNAME, clName, 22, "user22", "SDB_INSERT_REPLACEONDUP", [{ 'age': 22, 'name': 'user22' }], 0 );

   //REST接口flag参数不兼容SDB_INSERT_RETURN_ID,报错显示为-6：输入非法参数
   checkFlagType( restcl, COMMCSNAME, clName, 33, "user3", "SDB_INSERT_RETURN_ID", [], -6 );

   //REST接口flag参数兼容SDB_INSERT_RETURNNUM带回单条插入成功的返回值,插入数据出现唯一索引冲突时：报错-38
   checkFlagType( restcl, COMMCSNAME, clName, 44, "user4", "SDB_INSERT_RETURNNUM", [{ 'age': 44, 'name': 'user4' }], -38 );
   checkFlagType( restcl, COMMCSNAME, clName, 44, "user44", "SDB_INSERT_RETURNNUM", [{ 'age': 44, 'name': 'user44' }], 0 );

   commDropCL( db, COMMCSNAME, clName );
}

function sendRequest ( cmd, content )
{
   var port = parseInt( COORDSVCNAME, 10 ) + 4;
   var url = "http://" + COORDHOSTNAME + ":" + port;
   var curl = "curl " + "-X  POST \"" + url + "\" -d \"" + content + "\" -H \"Accept: application/json\" 2>/dev/null";
   var rc = cmd.run( curl );
   var rcObj = JSON.parse( rc );
   return rcObj;
}

function checkFlagType ( cl, COMMCSNAME, clName, userage, username, flagType, expData, expError )
{
   if( expError === undefined ) { expError = null; }
   if( expData === undefined ) { expData = []; }
   var cmd = new Cmd();
   var data = "{'name':'" + username + "','age': " + userage + "}";
   var content = "cmd=insert&name=" + COMMCSNAME + "." + clName + "&insertor=" + data + "&flag=" + flagType;
   var rc = sendRequest( cmd, content );
   var errno = rc[0]["errno"];
   if( expError != errno )
   {
      throw new Error( " check flag error ！" );
   }
   else
   {
      switch( errno )
      {
         case -6:
            if( flagType != "SDB_INSERT_RETURN_ID" )
            {
               throw new Error( "check SDB_INSERT_RETURN_ID errno !" );
            }
            break;
         case -38:
            if( flagType != "SDB_INSERT_RETURNNUM" || username != "user4" )
            {
               throw new Error( "check SDB_INSERT_RETURNNUM errno !" );
            }
            break;
         case 0:
            var cur = cl.find( { 'name': username } );
            commCompareResults( cur, expData );
            //校验兼容SDB_INSERT_RETURNNUM带回单条插入成功的返回值{ "InsertedNum": 1, "DuplicatedNum": 0 }
            if( rc[1] !== undefined && rc[1] != null && rc[1]["InsertedNum"] != 1 && rc[1]["DuplicatedNum"] != 0 )
            {
               throw new Error( "check SDB_INSERT_RETURNNUM result errno"
                  + "\nexp:{ 'errno': 0 }{ 'InsertedNum': 1, 'DuplicatedNum': 0 }"
                  + "\nact:{ 'errno': " + errno + "}{ 'InsertedNum': " + rc[1]["InsertedNum"] + ", 'DuplicatedNum': " + rc[1]["DuplicatedNum"] + " }" );
            }
            break;
      }
   }
}







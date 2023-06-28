The directory tree is as follow:
```
./mongotools/
├── commlib.js
├── config
│   ├── bigData.json
│   ├── bsonType.json
│   ├── index.json
│   ├── misc.json
│   └── shard.json
├── mongodump.js
├── Readme.md
└── tools
    ├── deploy.sh
    ├── mgodatagen
    ├── mongo
    ├── mongod
    ├── mongodump
    ├── mongorestore
    └── mongos
```

Tools version:
- mgodatagen=0.11.2 [Download](https://github.com/feliixx/mgodatagen/releases)
    - download in github feliixx/mgodatagen repository
- mongo,mongod,mongos=4.4.x community server
    - download in mongodb official website 
- mongodump,mongorestore=4.2.x community server
    - download in mongodb official website, this package includes the two tools
- sequoiadbfap=fap3

To run this js test, follow the steps:

1. check all the tools are in tools directory as tree showed before, and change files mode.

2. move your current directory to "mongotools/" in linux command line
```
> cd /data/sequoiadb/testcase_new/manual/mongotools
```

3. run deploy script to deploy a shard replset in MongoDB.
```
> ./tools/deploy.sh
```

4. deploy a SequoiaDB replset, with coord node's fap3 port in 50007.

5. run js test file.
```
> cd /data/sequoiadb/testcase_new/manual/mongotools
> /data/sequoiadb/bin/sdb -f ./mongo_dump_restore.js
```

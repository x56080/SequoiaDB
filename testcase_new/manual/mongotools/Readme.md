The directory tree is as follow:
```
./mongotools/
├── commlib.js
├── config
│   ├── bigData.json
│   ├── bsonType.json
│   ├── collector.conf
│   ├── index.json
│   ├── misc.json
│   └── shard.json
├── mongo_dump_restore.js
├── mongo_shake_replSet.js
├── mongo_shake_shard.js
├── Readme.md
└── tools
    ├── collector.linux
    ├── deploy.sh
    ├── mgodatagen
    ├── mongo
    ├── mongod
    ├── mongodump
    ├── mongorestore
    └── mongos
```

## Tools version:
- mgodatagen=0.11.2 [Download](https://github.com/feliixx/mgodatagen/releases)
    - download in github feliixx/mgodatagen repository
- mongo,mongod,mongos=4.4.x community server
    - download in mongodb official website 
- mongodump,mongorestore=4.2.x community server
    - download in mongodb official website, this package includes the two tools
- sequoiadbfap=fap3
- mongoshake=2.8.4 [Download](https://github.com/alibaba/MongoShake/releases)
    - download in github alibaba/MongoShake repository

## Testcases
### mongorestore/dump

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


### mongoshake

1. check mongoShake collector.linux is in tools directory as tree showed before, and change files mode.

2. move your current directory to "mongotools/" in linux command line
```
> cd /data/sequoiadb/testcase_new/manual/mongotools
```

3. run deploy script to deploy MongoDB, you can chose different deploy mode( sharded/replSet/standalone )
```
> ./tools/deploy.sh
```

4. deploy a SequoiaDB replset, with coord node's fap3 port in 50007.

5. according to the deployment of source DB and target DB, complete the following fields in collector.conf
- mongo_urls
- mongo_cs_url
- mongo_s_url
- tunnel.address

6. run different mongoshake js files.
```
> cd /data/sequoiadb/testcase_new/manual/mongotools
> /data/sequoiadb/bin/sdb -f ./mongo_shake_replSet.js
> /data/sequoiadb/bin/sdb -f ./mongo_shake_shard.js
```

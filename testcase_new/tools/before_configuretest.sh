#!/bin/bash
set -eo pipefail

COORDHOSTNAME="${1:-localhost}"
COORDSVCNAME="${2:-11810}"

currentHostname=$(hostname)

source /etc/profile >/dev/null 2>&1 || true
source /etc/default/sequoiadb

if ! ${INSTALL_DIR}/bin/sdblist -p 36100 >/dev/null 2>&1; then

    # remove tmp coord if exists
    if ${INSTALL_DIR}/bin/sdblist -p 18800 -m local >/dev/null 2>&1; then
        ${INSTALL_DIR}/bin/sdb "var tmpOma = new Oma(\"$COORDHOSTNAME\", 11790)"
        ${INSTALL_DIR}/bin/sdb "tmpOma.removeCoord(18800)"
        ${INSTALL_DIR}/bin/sdb "tmpOma.close()"
    fi

    # install datasource cluster
    cd "${INSTALL_DIR}/tools/deploy/"

    cat > sequoiadb.conf <<EOF
role,groupName,hostName,serviceName,dbPath
catalog,SYSCatalogGroup,$currentHostname,36000,[installPath]/database/catalog/36000
coord,SYSCoord,$currentHostname,36100,[installPath]/database/coord/36100
data,group1,$currentHostname,36200,[installPath]/database/data/36200
data,group2,$currentHostname,36300,[installPath]/database/data/36300
data,group3,$currentHostname,36400,[installPath]/database/data/36400
EOF

    bash quickDeploy.sh --sdb
fi

# create datasource in rootsite sdb

${INSTALL_DIR}/bin/sdb "var tmpDB = new Sdb(\"$COORDHOSTNAME\", $COORDSVCNAME)"

# check datasource exists
if ! ${INSTALL_DIR}/bin/sdb 'tmpDB.getDataSource("sdbscheduledatasource")' >/dev/null 2>&1; then
    ${INSTALL_DIR}/bin/sdb "tmpDB.createDataSource(\"sdbscheduledatasource\", \"$currentHostname:36100\", \"sdbadmin\", \"sdbadmin\")"
fi

${INSTALL_DIR}/bin/sdb 'tmpDB.close()'

# deploy sdb-schedule tools

toolsPath="${INSTALL_DIR}/tools/sdb-schedule/"

cd $toolsPath

# unzip sdb-schedule tools
tar -zxvf sdb-schedule-*-release.tar.gz

chmod +x sdb-schedule/bin -R

cd sdb-schedule/script/

cat > quickDeploy.conf <<EOF
rootsite_url="$COORDHOSTNAME:$COORDSVCNAME"
rootsite_user="sdbadmin"
rootsite_password="sdbadmin1"
node_list="9001,9002"
datasource_list="sdbscheduledatasource"
system_cs_name="SDB_SCHEDULE_SYSTEM"
system_cs_domain=""
EOF

echo 'y' | bash quickDeploy.sh deploy


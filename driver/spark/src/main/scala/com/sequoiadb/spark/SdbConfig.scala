/*
 * Copyright 2017 SequoiaDB Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

package com.sequoiadb.spark

import java.io.{File, FileInputStream}
import com.sequoiadb.net.ConfigOptions
import org.bson.BSONObject
import org.bson.util.JSON
import com.sequoiadb.util.{SdbDecrypt, SdbDecryptUserInfo}

import java.util.Properties
import scala.collection.mutable
import scala.collection.mutable.ArrayBuffer

/**
  * SequoiaDB configurations
  *
  * @param properties configurations in Map
  */
class SdbConfig(val properties: Map[String, String]) extends Serializable {
    private def notFound(name: String): Nothing = {
        throw new SdbException(s"Parameter $name is not specified")
    }

    private def invalidConfigValue(name: String, value: Any): Nothing = {
        throw new SdbException(s"Invalid value of parameter $name: $value")
    }

    require(
        SdbConfig.RequiredProperties.forall(properties.isDefinedAt),
        s"Not all required properties are defined! : ${
            SdbConfig.RequiredProperties.diff(
                properties.keys.toList.intersect(SdbConfig.RequiredProperties))
        }")

    // don't show password
    override def toString: String = (properties -- Seq(SdbConfig.Password)).toString()

    val host: List[String] = properties
        .getOrElse(SdbConfig.Host, notFound(SdbConfig.Host))
        .split(",").toList

    val collectionSpace: String = properties
        .getOrElse(SdbConfig.CollectionSpace, notFound(SdbConfig.CollectionSpace))

    val collection: String = properties
        .getOrElse(SdbConfig.Collection, notFound(SdbConfig.Collection))

    var username: String = properties
        .getOrElse(SdbConfig.Username, SdbConfig.DefaultUsername)

    val passwordType: String = properties
        .getOrElse(SdbConfig.PasswordType, SdbConfig.DefaultPasswordType)
    if (passwordType != SdbConfig.PASSWORD_TYPE_FILE
        && passwordType != SdbConfig.PASSWORD_TYPE_CLEARTEXT) {
        invalidConfigValue(SdbConfig.PasswordType, passwordType)
    }

    val token: String = properties
        .getOrElse(SdbConfig.Token, SdbConfig.DefaultToken)

    var password: String = properties
        .getOrElse(SdbConfig.Password, SdbConfig.DefaultPassword)

    if (passwordType == SdbConfig.PASSWORD_TYPE_FILE) {
        val sdbDecrypt: SdbDecrypt = new SdbDecrypt()
        val userInfo: SdbDecryptUserInfo = sdbDecrypt.parseCipherFile(username,
            token, new File(password))
        password = userInfo.getPasswd()
        username = userInfo.getUserName()
    }

    val samplingRatio: Double = properties.get(SdbConfig.SamplingRatio)
        .map(_.toDouble).getOrElse(SdbConfig.DefaultSamplingRatio)

    if (samplingRatio <= 0 || samplingRatio > 1.0) {
        invalidConfigValue(SdbConfig.SamplingRatio, samplingRatio)
    }

    val samplingNum: Long = properties.get(SdbConfig.SamplingNum)
        .map(_.toLong).getOrElse(SdbConfig.DefaultSamplingNum)

    if (samplingNum <= 0) {
        invalidConfigValue(SdbConfig.SamplingNum, samplingNum)
    }

    /**
      * sample _id for schema if true, ignore _id if false.
      */
    val samplingWithId: Boolean = properties.get(SdbConfig.SamplingWithId)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultSamplingWithId)

    /**
      * use single partition mode for schema sampling if true, use partitionMode if false.
      */
    val samplingSingle: Boolean = properties.get(SdbConfig.SamplingSingle)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultSamplingSingle)

    /**
      * bulk size when insert into SequoiaDB collection
      */
    val bulkSize: Int = properties.get(SdbConfig.BulkSize)
        .map(_.toInt).getOrElse(SdbConfig.DefaultBulkSize)

    if (bulkSize <= 0) {
        invalidConfigValue(SdbConfig.BulkSize, bulkSize)
    }

    val cursorType: String = properties
        .getOrElse(SdbConfig.CursorType, SdbConfig.DefaultCursorType)

    cursorType.toLowerCase match {
        case SdbConfig.CURSOR_TYPE_FAST =>
        case SdbConfig.CURSOR_TYPE_NORMAL =>
        case _ =>
            invalidConfigValue(SdbConfig.CursorType, cursorType)
    }

    val fastCursorBufSize: Int = properties.get(SdbConfig.FastCursorBufSize)
        .map(_.toInt).getOrElse(SdbConfig.DefaultFastCursorBufSize)

    if (fastCursorBufSize <= 0) {
        invalidConfigValue(SdbConfig.FastCursorBufSize, fastCursorBufSize)
    }

    val fastCursorDecoderNum: Int = properties.get(SdbConfig.FastCursorDecoderNum)
        .map(_.toInt).getOrElse(SdbConfig.DefaultFastCursorDecoderNum)

    if (fastCursorDecoderNum <= 0 || fastCursorDecoderNum > 16) {
        invalidConfigValue(SdbConfig.FastCursorDecoderNum, fastCursorDecoderNum)
    }

    private val scanType: String = properties
        .getOrElse(SdbConfig.ScanType, SdbConfig.SCAN_TYPE_AUTO)

    val partitionMode: String = properties
        .getOrElse(SdbConfig.PartitionMode, {
            // compatible with old edition option
            scanType.toLowerCase match {
                case SdbConfig.SCAN_TYPE_IXSCAN =>
                    SdbConfig.PARTITION_MODE_SHARDING
                case SdbConfig.SCAN_TYPE_TBSCAN =>
                    SdbConfig.PARTITION_MODE_DATABLOCK
                case _ => SdbConfig.DefaultPartitionMode
            }
        })

    partitionMode.toLowerCase match {
        case SdbConfig.PARTITION_MODE_SINGLE =>
        case SdbConfig.PARTITION_MODE_SHARDING =>
        case SdbConfig.PARTITION_MODE_DATABLOCK =>
        case SdbConfig.PARTITION_MODE_AUTO =>
        case _ =>
            invalidConfigValue(SdbConfig.PartitionMode, partitionMode)
    }

    val partitionBlockNum: Int = properties.get(SdbConfig.PartitionBlockNum)
        .map(_.toInt).getOrElse(SdbConfig.DefaultPartitionBlockNum)

    if (partitionBlockNum <= 0) {
        invalidConfigValue(SdbConfig.PartitionBlockNum, partitionBlockNum)
    }

    val partitionMaxNum: Int = properties.get(SdbConfig.PartitionMaxNum)
        .map(_.toInt).getOrElse(SdbConfig.DefaultPartitionMaxNum)

    // zero means no limit for partition max num when partitionMode is Datablock
    if (partitionMaxNum < 0) {
        invalidConfigValue(SdbConfig.PartitionMaxNum, partitionMaxNum)
    }

    val preferredLocation: Boolean = properties.get(SdbConfig.PreferredLocation)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultPreferredLocation)

    private val preferredInstanceMode: PreferredInstanceMode = properties
        .getOrElse(SdbConfig.PreferredInstanceMode, SdbConfig.DefaultPreferredInstanceMode).toLowerCase match {
            case SdbConfig.PREFERRED_INSTANCE_MODE_RANDOM => PreferredInstanceMode.Random
            case SdbConfig.PREFERRED_INSTANCE_MODE_ORDERED => PreferredInstanceMode.Ordered
            case s: String =>
                invalidConfigValue(SdbConfig.PreferredInstanceMode, s)
        }

    private val preferredInstanceStrict: Boolean = properties.get(SdbConfig.PreferredInstanceStrict)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultPreferredInstanceStrict)

    private val _preferredInstance: Array[String] = properties
        .getOrElse(SdbConfig.PreferredInstance, SdbConfig.DefaultPreferredInstance)
        .split(",").map(_.trim)

    for (preferred <- _preferredInstance) {
        preferred.toUpperCase match {
            case SdbConfig.PREFERRED_INSTANCE_MASTER =>
            case SdbConfig.PREFERRED_INSTANCE_SLAVE =>
            case SdbConfig.PREFERRED_INSTANCE_ANY =>
            case s: String => {
                try {
                    val id = s.toInt
                    if (id <= 0 && id > 255) {
                        invalidConfigValue(SdbConfig.PreferredInstance, _preferredInstance.mkString(","))
                    }
                } catch {
                    case _: Throwable =>
                        invalidConfigValue(SdbConfig.PreferredInstance, _preferredInstance.mkString(","))
                }
            }
        }
    }

    val preferredInstance: SdbPreferredInstance =
        new SdbPreferredInstance(_preferredInstance, preferredInstanceMode, preferredInstanceStrict)

    val shardingPartitionSingleNode: Boolean = properties.get(SdbConfig.ShardingPartitionSingleNode)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultShardingPartitionSingleNode)

    val ignoreDuplicateKey: Boolean = properties.get(SdbConfig.IgnoreDuplicateKey)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultIgnoreDuplicateKey)

    val ignoreNullField: Boolean = properties.get(SdbConfig.IgnoreNullField)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultIgnoreNullField)

    val useSelector: String = properties
        .getOrElse(SdbConfig.UseSelector, SdbConfig.DefaultUseSelector)

    useSelector.toLowerCase match {
        case SdbConfig.USE_SELECTOR_ENABLE =>
        case SdbConfig.USE_SELECTOR_DISABLE =>
        case SdbConfig.USE_SELECTOR_AUTO =>
        case _ =>
            invalidConfigValue(SdbConfig.UseSelector, useSelector)
    }

    val selectorDiff: Int = properties.get(SdbConfig.SelectorDiff)
        .map(_.toInt).getOrElse(SdbConfig.DefaultSelectorDiff)

    if (selectorDiff < 0) {
        invalidConfigValue(SdbConfig.SelectorDiff, selectorDiff)
    }

    val pageSize: Int = properties.get(SdbConfig.PageSize)
        .map(_.toInt).getOrElse(SdbConfig.DefaultPageSize)

    if (pageSize < 0) {
        invalidConfigValue(SdbConfig.PageSize, pageSize)
    }

    val lobPageSize: Int = properties.get(SdbConfig.LobPageSize)
        .map(_.toInt).getOrElse(SdbConfig.DefaultLobPageSize)

    if (lobPageSize < 0) {
        invalidConfigValue(SdbConfig.LobPageSize, lobPageSize)
    }

    val domain: String = properties
        .getOrElse(SdbConfig.Domain, SdbConfig.DefaultDomain)

    val shardingKey: String = properties
        .getOrElse(SdbConfig.ShardingKey, SdbConfig.DefaultShardingKey)

    try {
        JSON.parse(shardingKey).asInstanceOf[BSONObject]
    } catch {
        case _: Exception => invalidConfigValue(SdbConfig.ShardingKey, shardingKey)
    }

    val shardingType: String = properties
        .getOrElse(SdbConfig.ShardingType, SdbConfig.DefaultShardingType)

    shardingType.toLowerCase match {
        case SdbConfig.SHARDING_TYPE_HASH =>
        case SdbConfig.SHARDING_TYPE_RANGE =>
        case SdbConfig.SHARDING_TYPE_NONE =>
            if (shardingKey != "") {
                invalidConfigValue(SdbConfig.ShardingType, shardingType)
            }
        case _ =>
            invalidConfigValue(SdbConfig.ShardingType, shardingType)
    }

    val clPartition: Int = properties.get(SdbConfig.CLPartition)
        .map(_.toInt).getOrElse(SdbConfig.DefaultCLPartition)

    if (clPartition < 0) {
        invalidConfigValue(SdbConfig.CLPartition, clPartition)
    }

    val replicaSize: Int = properties.get(SdbConfig.ReplicaSize)
        .map(_.toInt).getOrElse(SdbConfig.DefaultReplicaSize)

    val compressionType: String = properties
        .getOrElse(SdbConfig.CompressionType, SdbConfig.DefaultCompressionType)

    compressionType.toLowerCase match {
        case SdbConfig.COMPRESSION_TYPE_SNAPPY =>
        case SdbConfig.COMPRESSION_TYPE_LZW =>
        case SdbConfig.COMPRESSION_TYPE_NONE =>
        case _ =>
            invalidConfigValue(SdbConfig.CompressionType, compressionType)
    }

    val autoSplit: Boolean = properties.get(SdbConfig.AutoSplit)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultAutoSplit)

    val group: String = properties
        .getOrElse(SdbConfig.Group, SdbConfig.DefaultGroup)

    val ensureShardingIndex: Boolean = properties.get(SdbConfig.EnsureShardingIndex)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultEnsureShardingIndex)

    val autoIndexId: Boolean = properties.get(SdbConfig.AutoIndexId)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultAutoIndexId)

    val autoIncrement: String = properties
        .getOrElse(SdbConfig.AutoIncrement, SdbConfig.DefaultAutoIncrement)

    try {
        JSON.parse(autoIncrement).asInstanceOf[BSONObject]
    } catch {
        case _: Exception => invalidConfigValue(SdbConfig.AutoIncrement, autoIncrement)
    }

    val strictDataMode: Boolean = properties.get(SdbConfig.StrictDataMode)
        .map(_.toBoolean).getOrElse(SdbConfig.DefaultStrictDataMode)

    val retryInstanceTimes: Int = SdbConfig.DefaultRetryInstanceTimes

    val retryInstanceInitDuration: Int = SdbConfig.DefaultRetryInstanceInitDuration

}

object SdbConfig {
    //  Parameter names
    val Host = "host"
    val CollectionSpace = "collectionspace"
    val Collection = "collection"
    val Username = "username"
    val PasswordType = "passwordtype"
    val Token = "token"
    val Password = "password"
    val SamplingRatio = "samplingratio"
    val SamplingNum = "samplingnum"
    val SamplingWithId = "samplingwithid"
    val SamplingSingle = "samplingsingle"
    val BulkSize = "bulksize"
    val CursorType = "cursortype"
    // fast, normal
    val FastCursorBufSize = "fastcursorbufsize"
    val FastCursorDecoderNum = "fastcursordecodernum"
    val PartitionMode = "partitionmode"
    // single, sharding, datablock, auto
    val PartitionBlockNum = "partitionblocknum"
    val PartitionMaxNum = "partitionmaxnum"
    val ShardingPartitionSingleNode = "shardingpartitionsinglenode"
    val PreferredLocation = "preferredlocation"
    val PreferredInstance = "preferredinstance"
    val PreferredInstanceMode = "preferredinstancemode"
    val PreferredInstanceStrict = "preferredinstancestrict"
    val IgnoreDuplicateKey = "ignoreduplicatekey"
    val IgnoreNullField = "ignorenullfield"
    val UseSelector = "useselector"
    val SelectorDiff = "selectordiff"
    val PageSize = "pagesize"
    val LobPageSize = "lobpagesize"
    val Domain = "domain"
    val ShardingKey = "shardingkey"
    val ShardingType = "shardingtype"
    val CLPartition = "clpartition"
    val ReplicaSize = "replsize"
    val CompressionType = "compressiontype"
    val AutoSplit = "autosplit"
    val Group = "group"
    val EnsureShardingIndex = "ensureshardingindex"
    val AutoIndexId = "autoindexid"
    val AutoIncrement = "autoincrement"
    val StrictDataMode = "strictdatamode"

    val ConfigPath = "configpath"

    // compatible with old edition option
    val ScanType = "scantype" // auto/ixscan/tbscan

    val CURSOR_TYPE_FAST = "fast"
    val CURSOR_TYPE_NORMAL = "normal"

    val PARTITION_MODE_SINGLE = "single"
    val PARTITION_MODE_SHARDING = "sharding"
    val PARTITION_MODE_DATABLOCK = "datablock"
    val PARTITION_MODE_AUTO = "auto"

    val PASSWORD_TYPE_CLEARTEXT = "cleartext"
    val PASSWORD_TYPE_FILE = "file"

    val SCAN_TYPE_IXSCAN = "ixscan"
    val SCAN_TYPE_TBSCAN = "tbscan"
    val SCAN_TYPE_AUTO = "auto"

    val PREFERRED_INSTANCE_MODE_RANDOM = "random"
    val PREFERRED_INSTANCE_MODE_ORDERED = "ordered"

    val PREFERRED_INSTANCE_MASTER = "M"
    val PREFERRED_INSTANCE_SLAVE = "S"
    val PREFERRED_INSTANCE_ANY = "A"

    val USE_SELECTOR_ENABLE = "enable"
    val USE_SELECTOR_DISABLE = "disable"
    val USE_SELECTOR_AUTO = "auto"

    val SHARDING_TYPE_NONE = "none"
    val SHARDING_TYPE_HASH = "hash"
    val SHARDING_TYPE_RANGE = "range"

    val COMPRESSION_TYPE_NONE = "none"
    val COMPRESSION_TYPE_SNAPPY = "snappy"
    val COMPRESSION_TYPE_LZW = "lzw"

    val AllProperties = List(
        Host,
        CollectionSpace,
        Collection,
        Username,
        PasswordType,
        Password,
        SamplingRatio,
        SamplingNum,
        SamplingWithId,
        SamplingSingle,
        BulkSize,
        CursorType,
        FastCursorBufSize,
        FastCursorDecoderNum,
        PartitionMode,
        PartitionBlockNum,
        PartitionMaxNum,
        ShardingPartitionSingleNode,
        PreferredLocation,
        PreferredInstance,
        PreferredInstanceMode,
        PreferredInstanceStrict,
        IgnoreDuplicateKey,
        IgnoreNullField,
        UseSelector,
        SelectorDiff,
        ScanType,
        PageSize,
        LobPageSize,
        Domain,
        ShardingKey,
        ShardingType,
        CLPartition,
        ReplicaSize,
        CompressionType,
        AutoSplit,
        Group,
        EnsureShardingIndex,
        AutoIndexId,
        AutoIncrement,
        StrictDataMode,
        ConfigPath)

    val RequiredProperties = List(
        Host,
        CollectionSpace,
        Collection)

    //  Default values
    val DefaultUsername = ""
    val DefaultPassword = ""
    val DefaultPasswordType = PASSWORD_TYPE_CLEARTEXT
    val DefaultToken = ""
    val DefaultSamplingRatio = 1.0
    val DefaultSamplingNum = 1000L
    val DefaultSamplingWithId = false
    val DefaultSamplingSingle = true
    val DefaultBulkSize = 500
    val DefaultCursorType: String = CURSOR_TYPE_FAST
    val DefaultFastCursorBufSize = 500
    val DefaultFastCursorDecoderNum = 2
    val DefaultPartitionMode: String = PARTITION_MODE_AUTO
    val DefaultPartitionBlockNum = 4
    val DefaultPartitionMaxNum = 1000
    val DefaultShardingPartitionSingleNode = false
    val DefaultPreferredLocation = false
    val DefaultPreferredInstanceMode: String = PREFERRED_INSTANCE_MODE_RANDOM
    val DefaultPreferredInstance: String = PREFERRED_INSTANCE_ANY
    val DefaultPreferredInstanceStrict: Boolean = true
    val DefaultIgnoreDuplicateKey = false
    val DefaultIgnoreNullField = false
    val DefaultUseSelector: String = USE_SELECTOR_ENABLE
    val DefaultSelectorDiff = 2

    // CS options
    val DefaultPageSize: Int = 1024 * 64
    val DefaultLobPageSize: Int = 1024 * 256
    val DefaultDomain = ""

    // CL options
    val DefaultShardingKey = ""
    val DefaultShardingType: String = SHARDING_TYPE_HASH
    val DefaultCLPartition = 1024
    val DefaultReplicaSize = 1
    val DefaultCompressionType: String = COMPRESSION_TYPE_NONE
    val DefaultAutoSplit = false
    val DefaultGroup = ""
    val DefaultEnsureShardingIndex = true
    val DefaultAutoIndexId = true
    val DefaultAutoIncrement = ""
    val DefaultStrictDataMode = false

    val DefaultConfigPath = ""

    // parameters for retry the selected node
    val DefaultRetryInstanceTimes = 3
    val DefaultRetryInstanceInitDuration = 75

    def apply(parameters: Map[String, String]): SdbConfig = {
        val configPath = parameters.getOrElse(SdbConfig.ConfigPath, "")
        var newParameters: Map[String, String] = parameters

        // 1. CHECK IF USES config file
        if (configPath != "") {
            val properties = new Properties()
            properties.load(new FileInputStream(configPath))

            val options = properties.propertyNames()
            while (options.hasMoreElements) {
                val optionName = options.nextElement().asInstanceOf[String]
                // 2. VALIDATE OPTIONS that config in file
                if (!SdbConfig.AllProperties.contains(optionName)) {
                    throw new SdbException(s"unsupported option: $optionName, please check!")
                }
                // 3. Do not overwrite, options
                if (!parameters.contains(optionName)) {
                    newParameters += (optionName -> properties.getProperty(optionName))
                }
            }
        }

        // 4. use new parameters to generate SdbConfig, it can be from file or CLI
        new SdbConfig(newParameters)
    }

    private[spark] val SdbConnectionOptions: ConfigOptions = {
        val opt = new ConfigOptions()
        opt.setConnectTimeout(3000)
        opt.setMaxAutoConnectRetryTime(0)
        opt.setSocketKeepAlive(true)
        opt
    }
}

class SdbPreferredInstance(val instances: Array[String], val mode: PreferredInstanceMode, val strict: Boolean) extends Serializable {
    private var _instanceIdArray: ArrayBuffer[Int] = mutable.ArrayBuffer()

    private var _instanceTendency: Option[String] = None

    for (instance <- instances) {
        instance.toUpperCase match {
            case SdbConfig.PREFERRED_INSTANCE_MASTER |
                SdbConfig.PREFERRED_INSTANCE_SLAVE |
                SdbConfig.PREFERRED_INSTANCE_ANY => if (_instanceTendency.isEmpty) {
                _instanceTendency = Option(instance.toUpperCase)
            }
            case s: String => {
                val id = s.toInt
                _instanceIdArray += id
            }
        }
    }

    val instanceIdArray: Array[Int] = _instanceIdArray.distinct.toArray

    def nonPreferred: Boolean = instanceIdArray.isEmpty &&
        (_instanceTendency.isEmpty ||
            _instanceTendency.get.equals(SdbConfig.PREFERRED_INSTANCE_ANY))

    def isPreferred: Boolean = !nonPreferred

    def hasInstanceTendency: Boolean = _instanceTendency.nonEmpty

    def hasInstanceId: Boolean = instanceIdArray.nonEmpty

    def isMasterTendency: Boolean = _instanceTendency.fold(false)(_.equals(SdbConfig.PREFERRED_INSTANCE_MASTER))

    def isSlaveTendency: Boolean = _instanceTendency.fold(false)(_.equals(SdbConfig.PREFERRED_INSTANCE_SLAVE))

    def isAnyTendency: Boolean = _instanceTendency.fold(true)(_.equals(SdbConfig.PREFERRED_INSTANCE_ANY))

    override def toString: String =
        s"{mode:$mode, instances: ${instanceIdArray.mkString("[", ",", "]")}, " +
            s"tendency: ${_instanceTendency.getOrElse(SdbConfig.PREFERRED_INSTANCE_ANY)}, strict:$strict}"
}

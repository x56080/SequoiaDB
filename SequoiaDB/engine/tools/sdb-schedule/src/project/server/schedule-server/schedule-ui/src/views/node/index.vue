<template>
  <div class="app-container">
    <!-- 搜索部分 -->
    <div class="search-box">
      <el-row :gutter="20" type="flex" justify="end">
        <el-col :span="9">
          <el-input
            maxlength="50"
            size="small"
            placeholder="节点主机名"
            v-model="searchParams.host_name"
            @keyup.enter.native="doSearch"
            clearable>
          </el-input>
        </el-col>
        <el-col :span="3">
          <el-button
            @click="doSearch"
            type="primary"
            size="small"
            icon="el-icon-search"
            style="width:100%">
            搜索
          </el-button>
        </el-col>
        <el-col :span="3">
          <el-button
            @click="resetSearch"
            size="small"
            icon="el-icon-circle-close"
            style="width:100%">
            重置
          </el-button>
        </el-col>
      </el-row>
    </div>

    <!-- 表格部分 -->
    <el-table
  v-loading="tableLoading"
  :data="tableData"
  border
  style="width: 100%; margin-top: 10px;"
>
  <el-table-column prop="host_name" label="主机名" width="300" show-overflow-tooltip></el-table-column>
  <el-table-column prop="ip_Addr" label="IP地址" width="300"></el-table-column>
  <el-table-column prop="port" label="端口" width="150" align="center"></el-table-column>

  <!-- 主节点标识 -->
  <el-table-column label="主节点" width="150" align="center">
    <template slot-scope="scope">
      <span v-if="scope.row.is_leader">是</span>
      <span v-else>-</span>
    </template>
  </el-table-column>

  <el-table-column prop="last_heart_time" label="最近一次心跳时间" width="300">
    <template slot-scope="scope">
      {{ formatTime(scope.row.last_heart_time) }}
    </template>
  </el-table-column>

  <el-table-column prop="status" label="状态" width="200" align="center">
    <template slot-scope="scope">
      <el-tag :type="getStatusTagType(scope.row.status)" size="mini">
        {{ getStatusText(scope.row.status) }}
      </el-tag>
    </template>
  </el-table-column>

  <el-table-column label="操作" width="269" align="center">
    <template slot-scope="scope">
      <el-button type="primary" size="mini" @click="handleShowDetail(scope.row)">
        查看
      </el-button>
    </template>
  </el-table-column>
</el-table>


    <!-- 分页 -->
    <el-pagination
      class="pagination"
      background
      layout="total, prev, pager, next, jumper"
      :total="pagination.total"
      :current-page.sync="pagination.current"
      :page-size="pagination.size"
      @current-change="handleCurrentChange">
    </el-pagination>

    <!-- 节点详情弹窗 -->
    <el-dialog
      title="节点详情"
      :visible.sync="detailDialogVisible"
      width="600px"
      :close-on-click-modal="false">
      <el-card shadow="hover" class="info-card">
        <h3 class="card-title">基本信息</h3>
        <el-row :gutter="20">
          <el-col :span="12"><strong>主机名：</strong>{{ currentNode.host_name }}</el-col>
          <el-col :span="12"><strong>IP 地址：</strong>{{ currentNode.ip_Addr }}</el-col>
          <el-col :span="12"><strong>端口：</strong>{{ currentNode.port }}</el-col>
          <el-col :span="12"><strong>租约数：</strong>{{ currentNode.lease_num }}</el-col>
          <el-col :span="12">
            <strong>主节点：</strong>
            <span v-if="currentNode.is_leader">是</span>
            <span v-else>否</span>
          </el-col>
          <el-col :span="12"><strong>状态：</strong>{{ getStatusText(currentNode.status) }}</el-col>
          <el-col :span="24"><strong>最近一次心跳时间：</strong>{{ formatTime(currentNode.last_heart_time) }}</el-col>
        </el-row>
      </el-card>

      <span slot="footer" class="dialog-footer">
        <el-button @click="detailDialogVisible = false">关闭</el-button>
      </span>
    </el-dialog>
  </div>
</template>

<script>
import { queryNodeList } from '@/api/node'

export default {
  name: 'NodeManage',
  data() {
    return {
      tableLoading: false,
      detailDialogVisible: false,
      currentNode: {},
      searchParams: { host_name: '' },
      pagination: { current: 1, size: 10, total: 0 },
      tableData: []
    }
  },
  methods: {
    fetchData() {
      this.tableLoading = true;
      const filter = {};
      if (this.searchParams.host_name && this.searchParams.host_name.trim() !== '') {
        filter.host_name = this.searchParams.host_name.trim();
      }

      queryNodeList(this.pagination.current, this.pagination.size, filter)
        .then(res => {
          this.tableData = res.data || [];
          this.pagination.total = Number(res.headers['x-record-count']) || this.tableData.length;
          this.tableLoading = false;
        })
        .catch(() => {
          this.tableLoading = false;
        });
    },
    doSearch() {
      this.pagination.current = 1;
      this.fetchData();
    },
    resetSearch() {
      this.searchParams.host_name = '';
      this.pagination.current = 1;
      this.fetchData();
    },
    handleCurrentChange(page) {
      this.pagination.current = page;
      this.fetchData();
    },
    handleShowDetail(node) {
      this.currentNode = JSON.parse(JSON.stringify(node));
      this.detailDialogVisible = true;
    },
    getStatusText(status) {
      switch (status) {
        case 'UP': return '运行中';
        case 'DOWN': return '已停止';
        case 'ABNORMAL': return '节点异常';
        default: return '-';
      }
    },
    getStatusTagType(status) {
      switch (status) {
        case 'UP': return 'success';
        case 'DOWN': return 'danger';
        case 'ABNORMAL': return 'danger';
        default: return 'info';
      }
    },
    formatTime(timestamp) {
      if (!timestamp) return '-';
      const date = new Date(timestamp);
      return date.toLocaleString();
    }
  },
  mounted() {
    this.fetchData();
  }
}
</script>

<style scoped>
.search-box {
  width: 50%;
  float: right;
  margin-bottom: 10px;
}

.info-card {
  margin-bottom: 15px;
  padding: 15px 20px;
  border-radius: 8px;
}

.card-title {
  font-size: 16px;
  font-weight: 600;
  margin-bottom: 10px;
  color: #409EFF;
}

.el-row {
  margin-bottom: 6px;
}

.el-col {
  margin-bottom: 6px;
}

.el-col strong {
  color: #606266;
}
</style>

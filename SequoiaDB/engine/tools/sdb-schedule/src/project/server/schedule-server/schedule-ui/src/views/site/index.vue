<template>
  <div class="app-container">
    <!-- 搜索部分 -->
    <div class="search-box">
      <el-row :gutter="20" type="flex" justify="end">
        <el-col :span="9">
          <el-input
            maxlength="50"
            size="small"
            placeholder="站点名称"
            v-model="searchParams.name"
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
      style="width: 100%; margin-top: 10px;">
      <el-table-column prop="name" label="站点名称" show-overflow-tooltip></el-table-column>
      <el-table-column label="数据源地址" show-overflow-tooltip>
        <template slot-scope="scope">
          {{ Array.isArray(scope.row.urls) ? scope.row.urls.join(', ') : '-' }}
        </template>
      </el-table-column>
      <el-table-column prop="user" label="用户名" show-overflow-tooltip></el-table-column>
      <el-table-column prop="datasource" label="数据源名" show-overflow-tooltip></el-table-column>
      <el-table-column label="操作" width="100" align="center">
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

    <!-- 站点详情弹窗 -->
    <el-dialog
      title="站点详情"
      :visible.sync="detailDialogVisible"
      width="600px"
      :close-on-click-modal="false">
      <el-card shadow="hover" class="info-card">
        <h3 class="card-title">基本信息</h3>
        <el-row :gutter="20">
          <el-col :span="12"><strong>站点名称：</strong>{{ currentSite.name }}</el-col>
          <el-col :span="12"><strong>数据源地址：</strong>{{ Array.isArray(currentSite.urls) ? currentSite.urls.join(', ') : '-' }}</el-col>
          <el-col :span="12"><strong>用户名：</strong>{{ currentSite.user }}</el-col>
          <el-col :span="12"><strong>数据源名：</strong>{{ currentSite.datasource }}</el-col>
        </el-row>
      </el-card>
      <span slot="footer" class="dialog-footer">
        <el-button @click="detailDialogVisible = false">关闭</el-button>
      </span>
    </el-dialog>
  </div>
</template>

<script>
import { querySiteList } from '@/api/site'

export default {
  data() {
    return {
      tableLoading: false,
      detailDialogVisible: false,
      currentSite: {},
      searchParams: { name: '' },
      pagination: { current: 1, size: 10, total: 0 },
      tableData: []
    };
  },
  methods: {
    fetchData() {
      this.tableLoading = true;
      let filter = {};
      if (this.searchParams.name && this.searchParams.name.trim() !== '') {
        filter.name = this.searchParams.name.trim();
      }
      querySiteList(this.pagination.current, this.pagination.size, filter)
        .then(res => {
          // 假设后端返回 { data: [...], headers: { 'x-record-count': total } }
          this.tableData = res.data;
          this.pagination.total = Number(res.headers['x-record-count']) || res.data.length;
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
      this.searchParams.name = '';
      this.pagination.current = 1;
      this.fetchData();
    },
    handleCurrentChange(page) {
      this.pagination.current = page;
      this.fetchData();
    },
    handleShowDetail(site) {
      this.currentSite = JSON.parse(JSON.stringify(site));
      this.detailDialogVisible = true;
    }
  },
  mounted() {
    this.fetchData();
  }
};
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

<template>
  <div class="app-container">
    <!-- 搜索栏 -->
    <!-- <div class="header-bar">
      <el-input
        v-model="searchKey"
        placeholder="搜索配置项"
        size="small"
        clearable
        @keyup.enter.native="handleSearch"
        style="width: 200px; margin-right: 10px;"
      />
      <el-button type="primary" size="small" icon="el-icon-search" @click="handleSearch">
        搜索
      </el-button>
      <el-button size="small" icon="el-icon-refresh" @click="handleReset">
        重置
      </el-button>
    </div> -->
    <div class="search-box">
      <el-row :gutter="20" type="flex" justify="end">
        <el-col :span="9">
          <el-input
            maxlength="50"
            size="small"
            placeholder="搜索配置项"
            v-model="searchKey"
            clearable
            @keyup.enter.native="handleSearch"
          />
        </el-col>

        <el-col :span="3">
          <el-button
            type="primary"
            size="small"
            icon="el-icon-search"
            style="width: 100%"
            @click="handleSearch"
          >
            搜索
          </el-button>
        </el-col>

        <el-col :span="3">
          <el-button
            size="small"
            icon="el-icon-circle-close"
            style="width: 100%"
            @click="handleReset"
          >
            重置
          </el-button>
        </el-col>
      </el-row>
    </div>

    <!-- 表格 -->
    <el-table
      v-loading="loading"
      :data="tableData"
      border
      stripe
      style="width: 100%; margin-top: 10px;"
    >
      <el-table-column prop="conf_key" label="配置项" />
      <el-table-column prop="conf_value" label="配置值" />
      <el-table-column prop="conf_desc" label="配置描述" />
      <el-table-column label="操作" align="center" width="120">
        <template slot-scope="scope">
          <el-button
            type="primary"
            size="mini"
            @click="handleEdit(scope.row)"
          >
            修改
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
      :current-page="pagination.current"
      @current-change="handlePageChange"
    />

    <!-- 修改配置弹窗 -->
    <el-dialog
      title="修改配置项"
      :visible.sync="editDialogVisible"
      width="400px"
      :close-on-click-modal="false"
    >
      <el-form label-width="80px" size="small">
        <el-form-item label="配置项">
          <el-input v-model="editForm.conf_key" disabled />
        </el-form-item>
        <el-form-item label="配置值">
          <el-input v-model="editForm.conf_value" placeholder="请输入配置值" />
        </el-form-item>
      </el-form>
      <div slot="footer" class="dialog-footer">
        <el-button @click="editDialogVisible = false">取 消</el-button>
        <el-button type="primary" @click="handleSave">保 存</el-button>
      </div>
    </el-dialog>
  </div>
</template>

<script>
import { queryGlobalConfList, updateConf } from '@/api/globalconf' // ✅ 你提供的接口方法

export default {
  name: 'SystemConfig',
  data() {
    return {
      loading: false,
      searchKey: '',
      tableData: [],
      pagination: { current: 1, size: 10, total: 0 },
      editDialogVisible: false,
      editForm: { key: '', value: '' },
    }
  },
  methods: {
    /** 查询接口 */
    fetchData() {
      this.loading = true
      const filter = {}
      if (this.searchKey.trim()) {
        // 按 key 模糊查询
        filter.conf_key = this.searchKey.trim()
      }

      queryGlobalConfList(this.pagination.current, this.pagination.size, filter)
        .then((res) => {
          // 假设返回的数据结构为 res.data = [{key:'aaa', value:'111'}]
          this.tableData = res.data || []
          // 假设后端返回总数在 headers 或 res.total
          this.pagination.total = Number(res.headers?.['x-record-count'] || res.total || this.tableData.length)
        })
        .catch(() => {
          this.$message.error('获取系统配置失败')
        })
        .finally(() => {
          this.loading = false
        })
    },

    /** 搜索 */
    handleSearch() {
      this.pagination.current = 1
      this.fetchData()
    },

    /** 重置 */
    handleReset() {
      this.searchKey = ''
      this.pagination.current = 1
      this.fetchData()
    },

    /** 分页切换 */
    handlePageChange(page) {
      this.pagination.current = page
      this.fetchData()
    },

    /** 点击编辑按钮 */
    handleEdit(row) {
      this.editForm = { ...row }
      this.editDialogVisible = true
    },

    /** 保存修改 */
    handleSave() {
      if (!this.editForm.conf_value.trim()) {
        this.$message.warning('配置值不能为空')
        return
      }

      this.saving = true
      // ✅ 调用修改接口
      updateConf(this.editForm.conf_key, this.editForm.conf_value)
        .then(() => {
          this.$message.success(`配置项「${this.editForm.conf_key}」已更新`)
          this.editDialogVisible = false
          this.fetchData() // 刷新列表
        })
        .catch(() => {
          this.$message.error('配置更新失败')
        })
        .finally(() => {
          this.saving = false
        })
    },
  },
  mounted() {
    this.fetchData()
  },
}
</script>

<style scoped>
.app-container {
  padding: 20px;
  background-color: #fff;
}
.header-bar {
  display: flex;
  align-items: center;
  justify-content: flex-end;
  margin-bottom: 10px;
}
.pagination {
  text-align: right;
  margin-top: 10px;
}
.dialog-footer {
  text-align: right;
}
.search-box {
  width: 50%;
  float: right;
  margin-bottom: 10px;
}

</style>

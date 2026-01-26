<template>
  <el-drawer
    class="match-result-drawer"
    title="匹配结果"
    :visible.sync="localVisible"
    direction="rtl"
    size="30%"
    :with-header="true"
    :modal="false"
    :append-to-body="true"
    @close="handleClose"
  >
    <el-tabs v-model="activeTab">
      <!-- 集合空间 -->
      <el-tab-pane label="集合空间" name="cs">
        <div class="scrollable-content">
          <div v-if="data.cs && data.cs.length">
            <div v-for="(item, idx) in data.cs" :key="idx">
              {{ item }}
            </div>
          </div>
          <div v-else>（无匹配）</div>
        </div>
      </el-tab-pane>

      <!-- 集合 -->
      <el-tab-pane label="集合" name="cl">
        <div class="scrollable-content">
          <div v-if="data.cl && data.cl.length">
            <div v-for="(item, idx) in data.cl" :key="idx">
              {{ item }}
            </div>
          </div>
          <div v-else>（无匹配）</div>
        </div>
      </el-tab-pane>
    </el-tabs>

  </el-drawer>
</template>

<script>
export default {
  name: "MatchResultDrawer",

  props: {
    visible: { type: Boolean, required: true },
    data: { type: Object, default: () => ({ cs: [], cl: [] }) }
  },

  data() {
    return {
      localVisible: this.visible,
      activeTab: "cs"   // 默认显示集合空间
    };
  },

  watch: {
    visible(val) {
      this.localVisible = val;
    },
    localVisible(val) {
      if (!val) this.$emit("update:visible", false);
    }
  },

  methods: {
    handleClose() {
      this.localVisible = false;
      this.$emit("update:visible", false);
    }
  }
};
</script>

<style scoped>
.scrollable-content {
  max-height: 900px;
  overflow-y: auto;
  padding: 5px 0;
}

.match-result-drawer ::v-deep .el-drawer__body {
  padding: 15px;
}
</style>

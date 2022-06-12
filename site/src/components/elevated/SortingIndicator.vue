<template>
  <div v-if="active" class="sorting-indicator" :style="{transform: 'rotate(' + (flipped ? '180deg' : '0') + ')'}">
    🔽
  </div>
</template>

<script>
export default {
  name: "SortingIndicator",
  props: {
    propertyName: String,
  },
  inject: ["sorting"],
  computed: {
    active() {
      return this.propertyName === this.sorting.on.value;
    },
    flipped() {
      return this.sorting.inverted.value;
    }
  },
}
</script>

<style scoped>
.sorting-indicator {
  display: inline-block;
  transition: transform 0.5s ease;
  transform-origin: center;
}
</style>

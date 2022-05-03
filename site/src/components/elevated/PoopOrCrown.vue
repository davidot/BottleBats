<template>
  <span v-if="hasCrown" class="poop-crown">
    👑
  </span>
  <span v-if="hasPoop" class="poop-crown">
    💩
  </span>
</template>

<script>
export default {
  name: "PoopOrCrown",
  props: {
    value: {},
    limits: Array,
  },
  computed: {
    hasCrown() {
      if (!this.limits || this.limits.length !== 2)
        return false;
      return this.limits[0] === this.value;
    },
    hasPoop() {
      if (!this.limits || this.limits.length !== 2)
        return false;
      return this.limits[1] === this.value;
    }
  }
}
</script>

<style scoped>
.poop-crown {
  opacity: 0.6;
  /*animation: fade-in 1s linear;*/
}

/*@keyframes fade-in {*/
/*  from {*/
/*    opacity: 0;*/
/*  }*/
/*}*/
</style>

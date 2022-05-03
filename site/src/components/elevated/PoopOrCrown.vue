<template>
  <span v-if="hasCrown" :class="['poop-crown', isRuben && 'spinning']">
    👑
  </span>
  <span v-if="hasPoop" :class="['poop-crown', isRuben && 'spinning']">
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
  inject: ["userDetails"],
  computed: {
    loggedIn() {
      return this.userDetails.values.value.displayName != null;
    },
    isRuben() {
      if (this.loggedIn) return this.userDetails.values.value.displayName === 'Ruben';
      return false;
    },
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

.spinning {
  animation: 0.5s spinning linear infinite;
}

@keyframes spinning {
  to {
    transform: scale(2.5);
    rotate: 360deg;
    opacity: 1;
  }
}
</style>

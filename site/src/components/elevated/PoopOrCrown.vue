<template>
  <span v-if="hasCrown" :class="['poop-crown', isRuben && 'spinning']"
        :style="{'animation-delay': (isRuben ? (Math.random() * 5) : '0') + 's', 'animation-duration': (isRuben ? (Math.random() * 4.0 + 2.1) : '0') + 's'}">
    👑
  </span>
  <span v-if="hasPoop" :class="['poop-crown', isRuben && 'spinning']"
        :style="{'animation-delay': (isRuben ? (Math.random() * 5.2) : '0') + 's', 'animation-duration': (isRuben ? (Math.random() * 10.5 + 17.3) : '0') + 's'}">
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
    isRuben() {
      return this.userDetails.values.value.isRuben;
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
  animation: spinning linear infinite;
}

@keyframes spinning {
  50% {
    transform: translateX(90px);

    /*transform: rotate3d(1, 2, -1, 192deg) scale(1.2);*/
    /*rotate: -90deg;*/
  }

  100% {
    /*transform: translateX(250px);*/
    transform: rotate3d(1, 2, -1, 192deg) scale(1.2);
    rotate: 360deg;
    opacity: 1;
  }
}
</style>

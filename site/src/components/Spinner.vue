<template>
  <div class="spin"></div>
</template>

<script>
export default {
  name: "Spinner",
};
</script>

<style scoped>

@keyframes spinning {
  to {
    rotate: 360deg;
  }
}
.spin {
  border: solid 5px gray;
  border-bottom-color: #5156bf;
  border-radius: 50%;
  aspect-ratio: 1 / 1;
  animation: spinning 2s linear infinite
}
</style>

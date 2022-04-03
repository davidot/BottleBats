<template>
  <div class="vijf-leaderboard-item">
    {{ data.name }} |
    <slot />
  </div>
</template>

<script>
export default {
  name: "VijfLeaderboardItem",
  props: {
    data: {},
  },
};
</script>

<style scoped>

.vijf-leaderboard-item {

}

</style>

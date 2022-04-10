<template>
  <div class="vijf-leaderboard-item">
    {{ data.name }} {{ data.played }}  {{ data.won }}|
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

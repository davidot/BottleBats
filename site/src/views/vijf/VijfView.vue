<template>
  <div class="vijf-nav">
    <router-link :to="{ path: '/vijf/' }">Leaderboard</router-link>
    <router-link :to="{ path: '/vijf/bots' }">Bots</router-link>
    <router-link :to="{ path: '/vijf/rules' }">Regels</router-link>
    <router-link :to="{ path: '/vijf/games' }">Games</router-link>
  </div>
  <router-view/>
</template>

<script>
export default {
  name: "VijfView",
};
</script>

<style scoped>
.vijf-nav {
  width: calc(70% + 20px);
  display: flex;
  flex-direction: row;
  align-content: center;
  align-items: center;
  margin: -10px 0 15px -10px;

  background-color: #787efb;
  /*gap: 10px;*/
  border-top: 2px solid white;
  border-bottom: 2px solid white;
  padding-left: 15%;
  padding-right: 15%;

}

.vijf-nav a {
  padding-top: 2px;
  flex-grow: 1;
  color: white;
  text-decoration: none;
  text-align: center;
  border-right: 2px solid white;
}

.vijf-nav a:first-child {
  border-left: 2px solid white;
}

a.router-link-exact-active {
  background-color: #444cf7;
}
</style>

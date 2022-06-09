<template>
  <main>
    <NavBar />
    <div class="main-content">
      <RouterView />
<!--      <VijfGame :start="start" :moves="moves" />-->
<!--      <LogoSVG style="width: 400px"/>-->
    </div>
  </main>
</template>

<script setup lang="ts">
import { RouterView } from "vue-router";
import VijfGame from "@/components/vijf/VijfGame.vue";
import NavBar from "@/components/NavBar.vue";
import {onMounted, provide, ref} from "vue";
import LogoSVG from "@/components/LogoSVG.vue";
import {endpoint} from "./http";

const start = ref("");
const moves = ref("");

// onMounted(() => {
//   setTimeout(() => {
start.value = "379;A6T;27Q;469;89Q;KAQ8KT428J3JT59+2A336545Q6K8*2KJA57*47JT;";
moves.value = "9A269K63+*Q";

// 0,18,false,2,32
// 2,19,false,5,0
// 3,20,false,1,0

// }, 1000);
// });

const userDetails = ref({
  displayName: null,
  isRuben: false,
  isAdmin: false,
});

function updateUserDetails() {
  endpoint.get("/auth/info").then((val) => {
      const userData = val.data;
      userDetails.value.displayName = userData.displayName;
      userDetails.value.isRuben = userData.displayName === "Ruben";
      userDetails.value.isAdmin = userData.admin;
  }).catch(() => {
      userDetails.value.displayName = null;
      userDetails.value.isAdmin = false;
      userDetails.value.isRuben = false;
  });
}

provide("userDetails", {
  values: userDetails,
  updateUserDetails,
});

updateUserDetails();
</script>

<style>
body {
  margin-top: 0;
  margin-bottom: 0;
  background-color: #d6dadb;
}

main {
  width: calc(min(1400px, max(800px, 80%)));
  margin: auto;
}

.bb-nav {
  position: sticky;
  top: 0;
  height: 50px;
  width: 100%;
  z-index: 100;
  background: #787efb;
  text-align: center;
  /*vertical-align: center;*/
}

.main-content {
  /*position: relative;*/
  /*top: 50px;*/
  background-color: #efefef;
  padding: 10px;
  min-height: calc(100vh - 70px);
}
</style>

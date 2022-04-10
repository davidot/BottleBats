<template>
  <div class="bb-nav">
    <LogoSVG style="float: left; height: 100%; background: purple; z-index: 1; border: 0;"/>
    <span class="nav-button" style="background: purple; color: black; border-left-width: 0;">
      Bottle Bats
    </span>

    <NavBarButton :to="{name: 'home'}">
      LeaderBoard
    </NavBarButton>
    <NavBarButton :to="{name: 'games'}">
      Games
    </NavBarButton>
    <NavBarButton :to="{name: 'bots'}">
      Bots
    </NavBarButton>

    <NavBarButton :to="{name: 'login'}" :clazz="'login-side'" v-if="!loggedIn">
      Log in / Register
    </NavBarButton>
    <span v-else class="nav-button login-side">
      <a @click="logOut">
        Logout
      </a>
    </span>
    <div v-if="loggedIn" class="nav-button login-side nav-active-link" style="cursor: default">
      Welcome, {{ username }}
    </div>
  </div>
</template>

<script>
import NavBarButton from "@/components/NavBarButton.vue";
import LogoSVG from "@/components/LogoSVG.vue";
import {endpoint} from "@/http";
export default {
  name: "TopBar",
  components: {LogoSVG, NavBarButton},
  inject: ["userDetails"],
  computed: {
    loggedIn() {
      return this.userDetails.values.value.displayName != null;
    },
    username() {
      if (this.loggedIn) return this.userDetails.values.value.displayName;
      return "Not logged in";
    },
  },
  methods: {
    logOut() {
      endpoint
          .post("/auth/logout", '')
          .then(() => {
            this.userDetails.updateUserDetails();
            console.log('Log out sucessful!');
          })
          .catch((err) => {
            console.log('Log out failed? probably just server disconnect?', err);
            this.userDetails.updateUserDetails();
          });
    }
  }
};
</script>

<style>

.nav-button {
  display: flex;
  justify-content: center;
  align-items: center;
  padding: 0 10px;
  float: left;

  height: 100%;
  background: #2831f3;
  border: 1px solid white;
  border-top: 0;
  border-bottom: 0;
  cursor: pointer;
}

.nav-button:hover {
  background-color: #6165f5;
}

.nav-button.nav-active-link {
  background-color: #b6b8f5;
}

.nav-button a {
  text-decoration: none;
  color: ghostwhite;
}

.nav-button.nav-active-link a {
  color: black;
}

.login-side {
  float: right;
}

</style>

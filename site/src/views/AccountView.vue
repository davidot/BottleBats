<template>
  <div class="login-box">
    <label for="username-input">Username</label>
    <input id="username-input" type="text" v-model="username" />
    <label
      for="token-input"
      title="(niet wachtwoord want dat is allemaal moeilijk)">
      Token
    </label>
    <input id="token-input" type="text" v-model="token" />
    <div style="margin: auto">
      <button :disabled="!hasValues" style="margin-right: 20px" @click="login">
        Log in
      </button>
      <button :disabled="!hasValues" @click="register">Register</button>
    </div>

    <div class="login-errors" v-show="errorText">
      {{ errorText }}
    </div>
  </div>
</template>

<script>
import { endpoint } from "@/http";

export default {
  name: "AccountView",
  inject: ["userDetails"],
  data() {
    return {
      errorText: null,
      username: "",
      token: "",
    };
  },
  computed: {
    hasValues() {
      return this.username.length > 0 && this.token.length > 0
          // && !this.username.includes(':') && !this.token.includes(':');
    },
  },
  methods: {
    login() {
      if (!this.hasValues) return;

      const username = this.username;
      const token = this.token;

      endpoint
        .get("/auth/login", {
          headers: {
            Authorization: username + ":" + token,
          },
        })
        .then((val) => {
          this.userDetails.updateUserDetails();
          this.$router.push({ name: "home" });
        })
        .catch(({ response }) => {
          this.errorText = response.data;
        });
    },
    register() {
      if (!this.hasValues) return;
    },
  },
};
</script>

<style scoped>
.login-box {
  display: flex;
  flex-direction: column;
  max-width: 200px;
}

.login-errors {
  color: red;
}
</style>

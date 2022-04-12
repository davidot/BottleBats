<template>
  <div class="login-view">
    <div class="login-box">
      <LogoSVG animating/>

      <label for="username-input">Username</label>
      <input id="username-input" type="text" v-model="username" @keyup="submitOnEnter" />
      <label
          for="token-input"
          title="(niet wachtwoord want dat is allemaal moeilijk)">
        Token
      </label>
      <input id="token-input" type="text" v-model="token" @keyup="submitOnEnter" />
      <div style="margin: auto">
        <button ref="logInButton" :disabled="!hasValues" style="margin-right: 20px" @click="login">
          Log in
        </button>
        <button :disabled="!hasValues" @click="register">Register</button>
      </div>

      <div class="login-errors" v-show="errorText">
        {{ errorText }}
      </div>

      <div v-if="loggedIn">
        <label>Nieuwe naam?</label><input type="text" v-model="newname">
        <button @click="rename">Ga nu snel</button>
      </div>
    </div>
  </div>
</template>

<script>
import { endpoint } from "@/http";
import LogoSVG from "@/components/LogoSVG.vue";

export default {
  name: "AccountView",
  components: {LogoSVG},
  inject: ["userDetails"],
  data() {
    return {
      errorText: null,
      username: "",
      token: "",
      newname: "",
    };
  },
  computed: {
    hasValues() {
      return this.username.length > 0 && this.token.length > 0
          && !this.username.includes(':') && !this.token.includes(':');
    },
    loggedIn() {
      return this.userDetails.values.value.displayName != null;
    },
  },
  methods: {
    rename() {
      if (!this.newname)
        return;

      const name = this.newname;
      endpoint
          .post("/auth/rename", name)
          .then(() => {
            this.userDetails.updateUserDetails();
            this.$router.push({ name: "home" });
          })
          .catch(({ response }) => {
            this.errorText = response.data;
          });
    },
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
        .then(() => {
          this.userDetails.updateUserDetails();
          this.$router.push({ name: "home" });
        })
        .catch(({ response }) => {
          this.errorText = response.data;
        });
    },
    register() {
      if (!this.hasValues) return;

      const username = this.username;
      const token = this.token;

      endpoint
          .post("/auth/register", '', {
            headers: {
              Authorization: username + ":" + token,
            },
          })
          .then(() => {
            this.userDetails.updateUserDetails();
            this.$router.push({ name: "rules" });
          })
          .catch(({ response }) => {
            this.errorText = response.data;
          });
    },
    submitOnEnter(event) {
      if (event.keyCode === 13 && this.hasValues)
        this.$refs.logInButton.click();
    }
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

.login-view {
  width: 100%;
  display: flex;
  flex-direction: row;
  justify-content: center;
}

.login-box > * {
  margin-top: 2px;
  margin-bottom: 2px;
}
</style>

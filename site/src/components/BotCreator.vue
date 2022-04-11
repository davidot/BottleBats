<template>
  <div style="display: flex; flex-direction: column; width: 100%; justify-items: center; justify-content: center; text-align: center">
    <div>
      <div
          :class="['file-drop-zone', dropActive && 'active']"
          @drop.prevent="onDrop"
          @dragenter.prevent="setActive"
          @dragover.prevent="setActive"
          @dragleave="setInactive"
          @click="$refs.fileInput.click"
      >
        <template v-if="!currentFile">
          Klik om te uploaden of sleep de file hier
        </template>
        <template v-else> File: {{ currentFile.name }} </template>
      </div>
      <input
          style="visibility: hidden"
          type="file"
          id="new-bot-file"
          ref="fileInput"
          @change="selectFile"
      />
    </div>
    <div>
      <label for="new-bot-name"> Naam: </label>
      <input type="text" id="new-bot-name" v-model="botName" />

      <button style="margin-left: 25px" :disabled="!currentFile || !botName || uploading" @click="uploadBot">
        Upload bot
      </button>
    </div>
    <div>

    </div>
    <span style="color: red">{{ lastError }}</span>
  </div>
</template>

<script>
import {endpoint} from "@/http";

export default {
  name: "BotCreator",
  data() {
    return {
      currentFile: null,
      lastError: "",
      dropActive: false,
      inActiveTimeout: null,
      botName: "",
      uploading: false,
    };
  },
  methods: {
    uploadBot() {
      if (!this.currentFile || !this.botName)
        return;

      this.uploading = true;

      const file = this.currentFile;
      const name = this.botName;

      const data = new FormData();
      data.append("file", file);
      data.append("name", name);

      endpoint.post("/vijf/upload", data).then((done) => {
        this.currentFile = null;
        this.uploading = false;
        this.$emit('new-bot');
      })
          .catch(({ response }) => {
            this.uploading = false;
            this.lastError = response.data;
          });
    },
    checkFile(file) {
      if (!file)
        return;

      let maxSize = 100000;
      if (file.size > maxSize) {
        this.lastError = `File is een beetje te groot, max ${
            maxSize / 1000
        }kb aub`;
        return;
      }

      this.currentFile = file;
      console.log(this.currentFile);
    },
    selectFile() {
      const files = this.$refs.fileInput.files;
      this.lastError = '';

      if (files.length !== 1) {
        this.lastError = "Precies 1 file graag, niet " + files.length;
        return;
      }

      this.checkFile(this.$refs.fileInput.files[0]);
    },
    onDrop(event) {
      const data = event.dataTransfer;
      this.dropActive = false;
      this.lastError = '';

      if (data.items.length > 1) {
        this.lastError = "Precies 1 file graag, niet " + data.items.length;
        return;
      }

      const item = data.items[0];

      if (item.kind !== "file") {
        this.lastError = "Een file graag, niet " + item.kind;
        return;
      }

      this.checkFile(item.getAsFile());
    },
    setActive() {
      clearTimeout(this.inActiveTimeout);
      this.dropActive = true;
    },
    setInactive() {
      this.inActiveTimeout = setTimeout(() => (this.dropActive = false), 50);
    },
  }
};
</script>

<style scoped>
.file-drop-zone {
  min-width: 100px;
  max-width: 50%;
  min-height: 40px;
  /*border: 2px solid black;*/
  /*border-radius: 10px;*/
  background-color: #b6b8f5;

  display: flex;
  justify-content: center;
  align-items: center;
  margin: auto;
}

.file-drop-zone.active {
  /*border-color: blue;*/
  background-color: #5156bf;
}
</style>

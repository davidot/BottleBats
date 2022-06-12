<template>
  <div style="display: flex; flex-direction: column; width: 100%; justify-items: center; justify-content: center; text-align: center">
    <div style="display: flex; flex-direction: row; align-self: center; gap: 40px; padding-bottom: 10px;">
      <div
          :class="['file-drop-zone', dropActive && 'active']"
          @drop.prevent="onDrop"
          @dragenter.prevent="setActive"
          @dragover.prevent="setActive"
          @dragleave="setInactive"
          @click="$refs.fileInput.click"
      >
        <div class="file-info">
          Klik om te uploaden of sleep de file hier
        </div>
        <div class="active-file" v-for="(file, index) of currentFiles" :key="file.name" @click.prevent.stop="currentFiles.splice(index, 1)">
          <img src="@/assets/file.png" style="width: 1em"/>
          {{ file.name }} ❌
        </div>
      </div>
      <input
          style="display: none"
          type="file"
          id="new-bot-file"
          ref="fileInput"
          :multiple="multiple"
          @change="selectFile"
      />
      <IconMaker v-if="withImage" ref="botImages"/>
    </div>
    <div>
      <label for="new-bot-name"> Naam: </label>
      <input type="text" id="new-bot-name" v-model="botName" />

      <button style="margin-left: 25px" :disabled="currentFiles.length === 0 || !botName || uploading" @click="uploadBot">
        Upload bot
      </button>
    </div>
    <span :style="{ 'color': failed ? 'red' : 'gray'}">{{ lastMessage }}</span>
  </div>
</template>

<script>
import {endpoint} from "@/http";
import IconMaker from "@/components/IconMaker.vue";

export default {
  name: "BotCreator",
  components: {IconMaker},
  props: {
    uploadUrl: String,
    multiple: {
      default: false,
      type: Boolean,
    },
    withImage: {
      default: false,
      type: Boolean,
    }
  },
  data() {
    return {
      currentFiles: [],
      anyDrawn: false,
      lastMessage: "",
      failed: false,
      dropActive: false,
      inActiveTimeout: null,
      botName: "",
      uploading: false,
    };
  },
  methods: {
    async uploadBot() {
      if (this.currentFiles.length === 0 || !this.botName)
        return;

      this.uploading = true;

      const files = this.currentFiles;
      const name = this.botName;

      const data = new FormData();
      data.append("name", name);

      if (this.withImage) {
        const imgBlob = await this.$refs.botImages.getBlob();
        if (imgBlob !== null)
          data.append("image", imgBlob);
      }

      let i = 0;
      for (let file of files)
        data.append("src_" + (i++), file);

      endpoint
        .post(this.uploadUrl, data)
        .then((done) => {
          this.lastMessage = done.data;
          this.failed = false;

          if (this.withImage)
            this.$refs.botImages.clear();

          this.currentFiles = [];
          this.uploading = false;
          this.$emit("new-bot");
        })
        .catch(({ response }) => {
          this.uploading = false;
          this.lastMessage = response.data;
          this.failed = true;
        });
    },
    addFile(file) {
      if (!file)
        return;

      let maxSize = 2500000;
      if (file.size > maxSize) {
        this.lastMessage = `File is een beetje te groot, max ${
            maxSize / 1000
        }kb aub`;
        return;
      }

      if (
        file.type.startsWith("image/")
      ) {
        const img = new Image();
        img.onload = () => {
          this.$refs.botImages.drawImage(img);
        };
        img.src = URL.createObjectURL(file);
        return;
      }

      if (!this.multiple) {
        this.currentFiles = [file];
      } else {
        this.currentFiles.push(file);
      }
    },
    selectFile() {
      const files = this.$refs.fileInput.files;
      this.lastMessage = "";

      if (!this.multiple && files.length !== 1) {
        this.lastMessage = "Precies 1 file graag, niet " + files.length;
        return;
      }

      for (let file of this.$refs.fileInput.files) {
        this.addFile(file);
      }
    },
    onDrop(event) {
      const data = event.dataTransfer;
      this.dropActive = false;
      this.lastMessage = "";

      if (!this.multiple && data.items.length > 1) {
        this.lastMessage = "Precies 1 file graag, niet " + data.items.length;
        return;
      }

      for (let item of data.items) {

        if (item.kind !== "file") {
          this.lastMessage = "Een file graag, niet " + item.kind;
          return;
        }

        this.addFile(item.getAsFile());
      }
    },
    setActive() {
      clearTimeout(this.inActiveTimeout);
      this.dropActive = true;
    },
    setInactive() {
      this.inActiveTimeout = setTimeout(() => (this.dropActive = false), 50);
    },
    useImage(e) {
      const src = e?.target?.src;
      if (!src)
        return;
      const img = new Image();

      img.onload = () => {
        this.$refs.botImages.drawImage(img);
      };
      img.src = src;
    }
  }
};
</script>

<style scoped>
.file-drop-zone {
  /*min-width: 100px;*/
  /*max-width: 50%;*/
  width: 100%;
  min-height: 40px;
  /*border: 2px solid black;*/
  /*border-radius: 10px;*/
  background-color: #b6b8f5;

  display: flex;
  justify-content: center;
  align-items: center;
  /*margin: auto;*/
  flex-wrap: wrap;
  padding: 4px;
  gap: 4px;
}

.file-drop-zone.active {
  /*border-color: blue;*/
  background-color: #5156bf;
}

.active-file {
  border: 2px solid #5156bf;
  border-radius: 5px;
  padding: 2px;
}

.active-file:hover {
  border-color: #fc2c2c;
  cursor: pointer;
}
</style>

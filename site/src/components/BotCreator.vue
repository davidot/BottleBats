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
          {{ file.name }}❌
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
      <canvas id="image-convert" width="124" height="124" ref="converter" style="border: black 1px solid; width: 124px; height: 124px;" @click="draw" @drag="draw" @mousemove="draw"/>
      <div style="width: 50px; height: 50px" :style="{'background-color': currentColor}"></div>
      <select v-model="currentColor">
        <option value="red">
          Rood
        </option>
        <option value="blue">
          Blauw
        </option>
      </select>
    </div>
    <div>
      <label for="new-bot-name"> Naam: </label>
      <input type="text" id="new-bot-name" v-model="botName" />

      <button style="margin-left: 25px" :disabled="currentFiles.length === 0 || !botName || uploading" @click="uploadBot">
        Upload bot
      </button>
    </div>
    <span style="color: red">{{ lastError }}</span>
  </div>
</template>

<script>
import {endpoint} from "@/http";

export default {
  name: "BotCreator",
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
  mounted() {
    const context = this.$refs.converter.getContext('2d');
    for (var i = 0; i < 31; i++) {
      context.fillStyle = `hsl(${i * 60}, 100%, 50%)`;
      context.fillRect(i * 4, i * 4, 4, 4);
    }
  },
  data() {
    return {
      currentFiles: [],
      currentColor: 'red',
      anyDrawn: false,
      lastError: "",
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

      if (this.anyDrawn) {
        const imgBlob = await new Promise(resolve => this.$refs.converter.toBlob(resolve, "image/png"));
        data.append("image", imgBlob);
      }

      let i = 0;
      for (let file of files)
        data.append("src_" + (i++), file);

      endpoint.post(this.uploadUrl, data).then((done) => {
        console.log('response', done.data);
        this.anyDrawn = false;
        this.$refs.converter.getContext('2d').clearRect(0, 0, 128, 128);
        this.currentFiles = [];
        this.uploading = false;
        this.$emit('new-bot');
      })
          .catch(({ response }) => {
            this.uploading = false;
            this.lastError = response.data;
          });
    },
    addFile(file) {
      if (!file)
        return;

      let maxSize = 2500000;
      if (file.size > maxSize) {
        this.lastError = `File is een beetje te groot, max ${
            maxSize / 1000
        }kb aub`;
        return;
      }

      // if (
      //   file.name.endsWith(".png") ||
      //   file.name.endsWith(".jpg") ||
      //   file.name.endsWith(".jpeg")
      // ) {
      //   console.log('converting!');
      //   const context = this.$refs.converter.getContext('2d');
      //   const img = new Image();
      //   img.onload = () => {
      //     context.drawImage(img, 0, 0, 128, 128);
      //     this.$refs.converter.toBlob(async (blob) => {
      //
      //       console.log('blob', await blob.text());
      //       }, "image/png");
      //     console.log(png);
      //     console.log(data);
      //   };
      //   img.src = URL.createObjectURL(file);
      //   return;
      // }

      if (!this.multiple) {
        this.currentFiles = [file];
      } else {
        this.currentFiles.push(file);
      }

    },
    selectFile() {
      const files = this.$refs.fileInput.files;
      this.lastError = '';

      if (!this.multiple && files.length !== 1) {
        this.lastError = "Precies 1 file graag, niet " + files.length;
        return;
      }

      for (let file of this.$refs.fileInput.files) {
        this.addFile(file);
      }
    },
    onDrop(event) {
      const data = event.dataTransfer;
      this.dropActive = false;
      this.lastError = '';

      if (!this.multiple && data.items.length > 1) {
        this.lastError = "Precies 1 file graag, niet " + data.items.length;
        return;
      }

      for (let item of data.items) {

        if (item.kind !== "file") {
          this.lastError = "Een file graag, niet " + item.kind;
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
    draw(event) {
      const context = this.$refs.converter.getContext('2d');
      context.fillStyle = this.currentColor;
      context.fillRect(4 * Math.floor(event.offsetX / 4), 4 * Math.floor(event.offsetY / 4), 4, 4);
      this.anyDrawn = true;
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

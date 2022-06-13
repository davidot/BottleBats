<template>
  <div>
    <canvas id="image-convert" :width="width" :height="height" ref="canvas" style="padding: 5px; border: black 3px dashed; width: 124px; height: 124px; cursor: crosshair"
            @mousedown="draw" @mouseup="draw" @mousemove="draw" @mouseleave="draw" @mouseenter="draw"
            @drop.prevent="drop"
            @dragenter.prevent=""
            @dragover.prevent=""/>
    <input type="color" @change="updateColor" style="display: none" ref="colorPicker"/>
    <div style="display: flex; flex-direction: row">
      <div style="margin-left: auto; margin-right: auto; width: 80%; height: 20px" :style="{'background-color': currentColor}" @click="$refs.colorPicker.click">
      </div>
      <button @click="clear">
        ❌
      </button>
    </div>
  </div>
</template>

<script>
export default {
  name: "IconMaker",
  mounted() {
    this.context2D = this.$refs.canvas.getContext("2d");
  },
  data() {
    return {
      width: 4 * 31,
      height: 4 * 31,
      currentColor: "#000000",
      context2D: null,
      anyDrawn: false,
      pressed: false,
      location: {
        x: -1,
        y: -1,
      },
      fillNum: 0,
    };
  },
  methods: {
    drawImage(img) {
      this.context2D.drawImage(img, 0, 0, this.width, this.height);
    },
    draw(event) {
      this.location.x = event.offsetX - 5;
      this.location.y = event.offsetY - 5;

      if (event.type === "mouseup" || event.type === "mouseleave") {
        this.pressed = false;
        return;
      }

      if (event.type === "mouseenter") {
        if ((event.buttons & 1) === 1)
          this.pressed = true;
        return;
      }

      this.context2D.fillStyle = this.currentColor;
      if (event.type === "mousedown") {
        this.pressed = true;
        this.anyDrawn = true;
        if (event.shiftKey) {
          this.context2D.fillRect(0, 0, this.width, this.height);
          return;
        }
        if (event.ctrlKey) {

          const red = parseInt(this.currentColor.slice(1, 3), 16);
          const green = parseInt(this.currentColor.slice(3, 5), 16);
          const blue = parseInt(this.currentColor.slice(5, 7), 16);
          let alpha = 255;
          if (this.currentColor.length > 7) {
            alpha = parseInt(this.currentColor.slice(7, 9), 16);
          }

          const uintColor = (alpha << 24) | (blue << 16) | (green << 8) | red;

          this.floodFill(
            this.context2D,
            this.location.x + 0,
            this.location.y + 0,
            uintColor
          );
          return;
        }

        this.context2D.fillRect(
          4 * Math.floor(this.location.x / 4),
          4 * Math.floor(this.location.y / 4),
          4,
          4
        );
      } else if (event.type === "mousemove" || event.type === "drag") {
        if (!this.pressed)
            return;

        this.anyDrawn = true;
        const start = [
          this.location.x - event.movementX,
          this.location.y - event.movementY,
        ];
        const step = [event.movementX, event.movementY];

        for (let i = 0; i < 1.0; i += 0.02) {
          const x = start[0] + i * step[0];
          const y = start[1] + i * step[1];

          this.context2D.fillRect(
              4 * Math.floor(x / 4),
              4 * Math.floor(y / 4),
              4,
              4
          );
        }

      }
    },
    getBlob() {
      if (!this.anyDrawn)
        return null;

        return new Promise(resolve => this.$refs.canvas.toBlob(resolve, "image/png"));
    },
    clear() {
      this.context2D.clearRect(0, 0, 128, 128);
      this.anyDrawn = false;
    },
    updateColor() {
      this.currentColor = this.$refs.colorPicker.value;
    },
    async floodFill(ctx, x, y, fillColor) {
      this.fillNum++;
      const thisFill = this.fillNum;

      function wait(delay = 1) {
        return new Promise((resolve) => {
          setTimeout(resolve, delay);
        });
      }

      function getPixel(pixelData, x, y) {
        if (x < 0 || y < 0 || x >= pixelData.width || y >= pixelData.height) {
          return -1;
        } else {
          return pixelData.data[y * pixelData.width + x];
        }
      }


      // read the pixels in the canvas
      const imageData = ctx.getImageData(0, 0, ctx.canvas.width, ctx.canvas.height);

      // make a Uint32Array view on the pixels so we can manipulate pixels
      // one 32bit value at a time instead of as 4 bytes per pixel
      const pixelData = {
        width: imageData.width,
        height: imageData.height,
        data: new Uint32Array(imageData.data.buffer),
      };

      // get the color we're filling
      const targetColor = getPixel(pixelData, x, y);

      let waits = 0;

      // check we are actually filling a different color
      if (targetColor !== fillColor) {

        const ticksPerUpdate = 80;
        let tickCount = 0;
        const pixelsToCheck = [x, y];
        while (pixelsToCheck.length > 0) {
          const y = pixelsToCheck.pop();
          const x = pixelsToCheck.pop();

          const currentColor = getPixel(pixelData, x, y);
          if (currentColor === targetColor) {
            pixelData.data[y * pixelData.width + x] = fillColor;

            // put the data back
            ctx.putImageData(imageData, 0, 0);
            ++tickCount;
            if (tickCount % ticksPerUpdate === 0) {
              waits++;
              if (waits >= 600 || this.fillNum !== thisFill)
                return;
              await wait();
            }

            pixelsToCheck.push(x + 1, y);
            pixelsToCheck.push(x - 1, y);
            pixelsToCheck.push(x, y + 1);
            pixelsToCheck.push(x, y - 1);
          }
        }
      }
    },
  },
}
</script>

<style scoped>

</style>

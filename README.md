# Bayer Filter GBRG

## Transform a Bayer image to RGB

A Bayer filter mosaic is a color filter array (CFA) for arranging RGB color filters on a square grid of photosensors. Its particular arrangement of color filters is used in most single-chip digital image sensors used in digital cameras, camcorders, and scanners to create a color imageCreate a bayer filter to achive greater optimization.

Using the following architecture, we were able to transform the computation from software to hardware, in the calcilation of the averages and thus achieving significat benefits in terms of performance:

![](/images/RTL.png)

---

The whole concept is that the hardware module takes as an input a single pixel, and when we start a new image. We need the start of an image and its size to determine the position of the pixel according to the image and consequently the color that the position represent in the Bayer form, and then calculate the averages and output the correct RGB values. 

![](/images/Image.png)

The averages are calculated accoriding to:

![](/images/average.png)

- If the pixel is green then , the red (blue) color is determined by the average of the two red (blue) adjacent pixels.
- If the pixel is blue then , the green (blue) color is determined by the average of the four green (blue) adjacent pixels.
- If the pixel is blue then , the green (red) color is determined by the average of the four green (red) adjacent pixels.


---

The hardware module is communicating with and axi4-stream protocol with logic: 

![](/images/axi.png)

and using the axi dma ip offered by xilinx :

![](/images/arch.png)

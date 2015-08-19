# PNGDecoder

<h2>Overview</h2>

<p>This is a basic PNG decoder I made for educational purposes. I really just wanted to see how involved the process is. I also wanted to make it open to others, so that it might help people who are also curious and might want to try their hands at a decoder. Therefore, parts of the code might be more heavily commented than I otherwise would've made it.</p>

<p>The decoder supports a PNG image that:</p>
<ul>
  <li>Is in TrueColor.</li>
  <li>Has a RGBA format (That is, each pixel is described by a red, green, and blue value with an additional alpha   component).</li>
  <li>Has a bit-depth (The number of bits for each color component) of 8.</li>
  <li>Is <b>not</b> interlaced.</li>
</ul>

<p>I wrote the software in C++, and made it in an object-oriented style.</p>

<h2>Compilation</h2>

<p>I haven't made any kind of fancy Cmake for the project. I've just uploaded the source code. Therefore, you'll need to create your own project with the dependencies (listed below) if you want to compile it. Also, the code is not completely cross-platform friendly. I use a few specific windows functions, however I've tried my best to make it easy to port if you are interested.</p>

<h2>Dependencies</h2>

<p>I've really only used one external library for this project, which is <b>zlib</b>. I use zlib to for the <i>inflate</i> algorithm, as the image data within a PNG file is decompressed using the zlib format.</p>
<p>You can download the zlib library at: http://www.zlib.net/</p>
<p>I used <b>Visual Studio 2015</b>, running <b>Windows 10</b> while creating this project. I have <b>not</b> tested it on any other machine.</p>


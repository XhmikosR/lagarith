*(Copy-paste description)*

Lagarith offers better compression than codecs like Huffyuv, Alparysoft, and CorePNG.
There are a few lossless codecs that can compress better than Lagarith, such as MSU and FFV1;
however Lagarith tends to be faster than these codecs.

Lagarith is able to operate in several colorspaces - RGB24, RGB32, RGBA, YUY2, and YV12.
For DVD video, the compression is typically only 10-30% better than Huffyuv. However,
for high static scenes or highly compressible scenes, Lagarith significantly outperforms Huffyuv.
Lagarith is able to outperform Huffyuv due to the fact that it uses a much better compression method.
Pixel values are first predicted using median prediction (the same method used when "Predict Median"
is selected in Huffyuv).
This results in a much more compressible data stream. In Huffyuv, this byte stream would
then be compress using Huffman compression. In Lagarith, the byte stream may be subjected
to a modified Run Length Encoding if it will result in better compression. The resulting
byte stream from that is then compressed using Arithmetic compression, which, unlike Huffman
compression, can use fractional bits per symbol. This allows the compressed size to be very
close to the entropy of the data, and is why Lagarith can compress simple frames much better
than Huffyuv, and avoid expanding high static video.

Additionally, Lagarith has support for null frames; if the previous frame is mathematically
identical to the current, the current frame is discarded and the decoder will simply use the
previous frame again.

Requirements:

* Windows XP SP3 or newer
* An SSE capable CPU

Notes:

* This is just an Git mirror I set up and nothing more, no code changes.
* All credits goes to the Lagarith developer.

# Introduction #

What you need:
  1. A multimedia file.
  1. A version of FFMPEG for your system. Window users refer to here: http://ffmpeg.arrozcru.org/wiki/index.php?title=Links.


# Instructions #

  1. To start, open a command prompt and navigate using the _cd_ command to the directory with your source media, your ffmpeg executable, and where your output media will be placed.
  1. Type the following:
`ffmpeg -i INPUT.abc -vframes 20 -s cif -pix_fmt rgb24 OUTPUT.rgb`
  1. Remember to replace INPUT.abc to your file-to-be-converted, change the -vframes 20 value to however many frames of the media you want converted (or remove for whole media), and optionally changing the OUTPUT.rgb name (leaving the filename extension .rgb intact).
  1. There you go! Assuming everything went well, your video file is ready for testing.
okay so this book setup ended up being a bit more complicated than i'd like lol

to compile the HTML version of the book, run `mdbook build` in this directory.
you'll need to have python3 installed and available under that name, as some of
the preprocessing scripts are written in python.

tested with mdbook v0.4.36, some of the below hacks might break with a newer one

the highlighting is a bit of a mess: theme/highlight.js is the file actually
used by mdbook. we can't add hljs-asar as a separate js file, because it'll be
ran too late. so we need to override mdbook's highlight.js file with our own.
this is done by update_hljs.py, which takes highlight.min.js and appends
hljs-asar.js to it. highlight.min.js itself is just downloaded from
https://highlightjs.org/download, with a minimal set of languages (plaintext,
bash, powershell, python) enabled.

this means you need to run update_hljs.py any time you either change the asar
highlighter or you upgrade the base highlight.min.js. note that i think mdbook's
js is so outdated it'll break with highlight.js v12, so watch out for that lol

the highlight color schemes are also kinda scuffed lol, i didn't like the
default colors so i copied some themes over from
https://github.com/highlightjs/base16-highlightjs. these weren't used as-is, for
all of them i removed the margin/padding on <code> tags because it seems
mdbook's styling does that by itself already. also commented out the ::selection
rules, they just broke selection highlights. more tweaks:

* theme/highlight.css is one-light.css with the comments made darker and in
  italics
* theme/tomorrow-night.css actually atelier-dune.css with the orange color
  tweaked a bit, and with italics comments
* theme/ayu-highlight.css is exactly the same as theme/tomorrow-night.css
  (should make a better one at some point, i actually liked the old ayu theme
  best but it didn't have enough different colors)

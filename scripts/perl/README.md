PERL scripts
====================

This directory contains PERL scripts which utilize Humdrum Extras (and/or Humdrum Toolkit)
commands.  These scripts are linked to the ../../bin directory when the Humdrum Extras
tools are compiled.  To see if they are accessible in your terminal, type:

```bash
which sortcount
```

The computer should reply with the location of the `sortcount` script.  If it replies
with nothing, then the scripts are not installed.


Brief descriptions
-------------------


| Tool name     | Description |
| ------------- | ------------- |
| finalis       | Identify the final note/chord root in an input file. |
| finalis-tonic | Add key designation according to the finalis note of input file. |
| gptab2str     | Convert Guitar Pro ASCII tab to **str representation. |
| hummath       | Perform mathematical operations on one or more spines. |
| introspect    | Search a work for repeated patterns that occur within the work. |
| lyrics        | Extract lyric text from **text spines in musical scores. |
| motion        | Count occurrences of similar, contrary and oblique motion in counterpoint modules. |
| partjoin      | Merge multiple parts into a single score (preserving grace notes). |
| sdmarkov      | Generate scale-degree markov transition tables from **kern data. |
| sortcount     | Shortcut for "sort \| uniq -c \| sort -nr".  Also can output vega-lite bar charts of input data. |
| webnote       | Generates a webpage with musical data created from input Humdrum data. |
| webscore      | Generates a webpage with musical data created from input Humdrum data. |





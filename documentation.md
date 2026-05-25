## Docs
`load <file>` - loads a syntax file so velte can parse it
`fileend [REGEX] <syntax name>` - links any file endings that match the syntax highlighting file
`syntax [COLOUR HEXCODE] [REGEX] (curr/sub)` - colours an expression that matches the regex with the colour. 'curr' indicates a new rule to the current syntax selected (eg. if a new rule was made in the command line), 'sub' indicates colouring within an already coloured expression (any submatches)

`display <displayConfig> [COLOUR HEXCODE]` - changes the colour of any component of velte, including:
    - `linenumtextcolour` - the colour of the line numbers
    - `linenumbertextactivecolour` - the colour of the line that the cursor is on
    - `linenumcolour` - the colour of the background of the line
    - `statustextcolour` - the colour of the status bar
    - `backgroundcolour` - the background colour
    - `messagecolour` - the colour of any messages that pop up on screen

`gotox <x/end>` - goes to the x position of a line
`gotoy <y/end>` - goes to the line number

`config <config> (arguments)` - changes configuration,
    - `disline <num>` - changes distance from edge of screen to the linenums
    - `tabcount <num>` - the width of a tab
    - `linenums <true/false>` - if the linenumbers should be displayed or not

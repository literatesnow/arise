Arise v2.0 - Copyright (C) 2004-2006 bliP
Web: http://nisda.net
Email: spawn [at] nisda [dot] net
IRC: #nzgames irc.enterthegame.com
Compiled: v1.1: 2005-01-20, v2.0: 2006-04-22

Arise displays customizable on screen popup messages, its main purpose is to
show chat or events on IRC. While it has been designed as a DLL for mIRC it
includes exports to be used in other situations.

Requires mIRC v6.17 but will probably work with older versions.

Copy arise.dll and arise.mrc into your \mIRC directory.
Open mIRC and type:
  /load -rs arise.mrc
Push Yes to run initialization commands.

Use arisex.dll if you need backwards compatibility with Arise v1.0 scripts.

Commands:
  /ar - toggles popups
  /arise_example - example

Tips:
  Right click a popup sticks it there until...
  Left click removes popup immediately

Customising Popups:
  Tools -> Script Editor -> Variables
  The variables are called %arise_<name> and the value is delimited by a single
  underscore: _

  %arise_text Verdana_8_2_0,0,0_Verdana_8_1_0,0,0_174,219,255_7000_1_0_40_100_1_300

  Verdana     - Title font name
  8           - Title font size
  2           - Title font style
  0,0,0       - Title font colour
  Verdana     - Body font name
  8           - Body font size
  1           - Body font style
  0,0,0       - Body font colour
  174,219,255 - Background colour
  7000        - On screen time (ms)
  1           - Window styles
  0           - Window position
  0           - Window transparency
  40          - Show animation
  100         - Show animation duration (ms)
  1           - Hide animation
  100         - Hide animation duration (ms)

  Notes:
    - Colour is in RGB format: Red 128, Green 255, Blue 255.
      Aqua is written as: 128,255,255 make sure there are no spaces.

    - On screen time is in milliseconds, 1 second is 1000 milliseconds.

    - Font style:
      Normal - 1
      Bold - 2
      Italic - 4
      Underline - 8
      To use more than one style at a time, add them together, for example:
      Bold and italic is a value of 6 (2+4). Bold, italic and underline would
      be 14 (2+4+8).

    - Window style:
      Border - 1
      Show at left side of screen - 2
      Show at right side of screen - 4
      Start at top of screen - 8
      Start at bottom of screen - 16
      To use more than on style at a time, add them together, for example:
      Window with a border and starting at top of screen is a value of 9 (1+8).

    - Window position:
      Popups can be displayed on any monitor, this value is the x co-ordinate of
      the monitor closest to this point.
      For example:
        Monitor 1: 1024x768 resolution
        Monitor 2: 800x600 resolution
        Value between 0 and 1023: Shown on monitor 1
        Value between 1024 and 1824: Shown on monitor 2

    - Transparency range is 1 from to 255 (almost invisible to solid).
      Unfortunately, transparency doesn't work well with some animations.

    - Animation style:
      Fade - 1
      Center - 2
      Roll - 4
      Slide - 8
      Left to right - 16
      Right to left - 32
      Top to bottom - 64
      Bottom to top - 128
      To use more than one style at a time, add them together, for example:
      Slide and right to left is a value of 40 (32+8). Some animations do
      not work well together or at all.

Scripting:
  For full customisation, basic mIRC scripting is required. Please refer to
  mIRC's help for more information and/or a tutorial from:
  http://www.mircscripts.org/
  You cannot trigger your own text events.

  Tools -> Script Editor -> Remote (choose arise.mrc from the view menu if it
  is not the current file)

  The $arise identifier displays popups
    $arise(<settings>,<title text>,<body text>)

  Popups are shown when certain events occur
    on 1:TEXT:*:#:{ $arise(%arise_text,$nick > $_userprefix $+ $chan,$1-) }

    on TEXT - Channel/private messages
    * - Matches all text
    # - From any channel (? means private message)
    $arise - Display a popup
    %arise_text - Text settings
    $nick - Person who talked (built in)
    $_userprefix - @ for op, % for half op, + for voice, nothing for normal
    $+ - Put too words together
    $chan - Channel text is from
    $1- - The text itself

  Sometimes you might not want popups to be displayed for a certain channel
    Only trigger for channels #foo and #bar but not #foobar
      on 1:TEXT:*:#foo,#bar:{ <snip> }
    To disable a trigger permanently comment it out with the ";" character
      ;on 1:QUIT:{ <snip> }

Example to add a special popup for nick highlight:
  Tools -> Script Editor -> Variables
    Add:

    %arise_high Tahoma_12_6_45,255,25_Arial_10_1_255,240,132_220,0,20_4500_0_0_128_2_200_68_300

    Tahoma, 12pt, bold, italic and light green colour title text.
    Arial, 10pt, normal and light yellow colour body text.
    Dark red background which stays on screen for 4.5 seconds.
    No border, displayed on primary monitor.
    50% transparent popup.
    When the popup is shown, there is a center in effect which lasts 200 milliseconds.
    When the popup is hidden, there is a top to bottom roll effect which lasts 200 milliseconds.
    
    It looks absolutely terrible but hopefully it's a good example.

  Tools -> Script Editor -> Remote (arise.mrc)
    Add (must be before the catch-all * on TEXT):

     on 1:TEXT:$(* $+ $me $+ *):*:{ $arise(%arise_high,$nick $_userprefix $+ $chan,$1-) }

    This will match your nickname.

  Push OK and wait for someone to say your name.

Technical Information:
  This DLL can be used by other programs and is not limited to mIRC.
  The following are the functions to call to display a popup from arise.dll,
  make sure that the DLL is always loaded so the popups can position themselves
  correctly.

  typedef int (APIENTRY *SHOWPOPUP)(
          HWND mWnd, HWND aWnd, char *data, char *parms, BOOL show, BOOL nopause
          );

  typedef int (APIENTRY *ARISE)(
          const char *tfont, int theight, int tstyle, const char *tcolour,
          const char *bfont, int bheight, int bstyle, const char *bcolour,
          const char *mcolour, int mtime, int mstyle, int mpoint,
          int malpha, int mshowani, int mshowtime, int mhideani, int mhidetime,
          const char *ttext, const char *btext
          );
 
  Example:
      SHOWPOPUP ShowPopup;
      ARISE Arise;

      ... load dll ...
      ShowPopup = (SHOWPOPUP)GetProcAddress(dll, "ShowPopup");
      Arise = (ARISE)GetProcAddress(dll, "Arise");

      ShowPopup(NULL, NULL, "Title Text_Body Text_"
                            "Verdana_8_2_0,0,0_Verdana_8_1_0,0,0_"
                            "174,219,255_7000_1_0_0_40_100_1_300",
                            "", 0, 0);

      Arise("Verdana", 8, 2, "0,0,0",
            "Verdana", 8, 1, "0,0,0",
            "174,219,255", 7000, 1, 0, 0, 40, 100, 1, 300,
            "Title Text", "Body Text");
      
     ... unload dll (when popup has finished) ... 

Please send all questions, bug reports, comments and suggestions
to the email address provided above.

Disclaimer:
ANY USE BY YOU OF THE SOFTWARE IS AT YOUR OWN RISK. THE SOFTWARE IS
PROVIDED FOR USE "AS IS" WITHOUT WARRANTY OF ANY KIND. THIS SOFTWARE
IS RELEASED AS "FREEWARE". REDISTRIBUTION IS ONLY ALLOWED IF THE
SOFTWARE IS UNMODIFIED, NO FEE IS CHARGED FOR THE SOFTWARE, DIRECTLY
OR INDIRECTLY WITHOUT THE EXPRESS PERMISSION OF THE AUTHOR.

-bliP
"Because I Can."

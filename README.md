# qtkit

A Qt Widgets program should look right on the desktop it runs on. On a
dark desktop that means a dark window -- and Qt does not manage that by
itself on every desktop. On one Qt is not integrated with, such as TDE
or LXQt, it hands the program its default light palette, so the window
glares white in a dark session and nothing reports a fault.

qtkit is the small set of pieces that make a Qt Widgets program behave
the way its user's desktop does, written once with each choice tested,
for programs that would otherwise each write their own.

**Its theme** decides whether the desktop is dark by asking, in order,
what the program's user chose, Qt itself, the freedesktop portal, the
files TDE, KDE and LXQt write, and only then the palette Qt was given --
which always answers, and answers light where Qt is not integrated, so
asking it first would hide every other source. It puts a complete dark
palette on the program, including the shading roles a style draws frames
and headers with, and follows the desktop when it changes. It never asks
the session bus when none is named, because doing so starts a bus daemon
that nothing stops.

**Its flow layout** puts a row of buttons on as many lines as the window
needs. A plain horizontal row can only squeeze its buttons until their
labels are cut, and keeps any dialog holding it at least as wide as all of
them together; this one wraps, gives the window the height the wrap needs,
and on a wide window lays out exactly as the plain row would.

How it differs from what programs do without it:

- **Qt alone** follows the desktop only where a platform theme connects
  it -- GNOME, Plasma, Windows, macOS. qtkit adds the desktops that write
  their colours to a file and say nothing else.
- **Setting a style sheet or a fixed dark palette** ignores what the user
  chose for their desktop; qtkit applies dark only when the desktop is
  dark, and returns to the user's own light theme rather than one of its
  own.
- **Copying another program's theme code** copies what that program had
  learned at the time; qtkit holds the best of each copy and its reasons.

## Using it

As a submodule, and one line in the program's `.pro`:

    QT += dbus                      # optional: the portal, and its signal
    include(qtkit/qtkit.pri)

then, after the `QApplication` exists:

    qtkit_follow_desktop(application);

## Building

    make          # libqtkit.a, so the tree builds on its own
    make test     # the suite, offscreen

`fmake` builds it as well as `make` does.

Copyright (C) 2026 Nabeel Sowan <nabeel@vibes.se>

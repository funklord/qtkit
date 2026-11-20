# qtkit

## What this is

**The Qt Widgets pieces every GUI in this workspace needs and each was
writing for itself**, gathered once, tested once, and vendored by the
programs that use them. Started 2026-11-20 at the holder's word, after a
survey of ten GUI trees found the dark-desktop detection written five
times (hydra, beerssh, snurra, situ, and beerssh's palette beside it),
each with a part the others lacked.

The aim: a new GUI here gets the behaviour the others have by including
one `.pri`, and a fix to that behaviour lands in every program that
takes the update, instead of in the one tree whose session noticed.

### What belongs here

- **A piece moves in when two trees have written it**, not when one
  might. What only one program uses stays in that program.
- **The best of the copies, not the first**: every copy is read, the
  parts compared, and the reasons for each choice written below. A copy
  picked arbitrarily would lose what the others learned.
- **Qt Widgets only.** No QML, no QtQuick (`harmonization.md`), and
  nothing that is not Qt: a low-level C kit, if one is ever earned, is
  another project.

### How it is consumed

As a git submodule, `vendor/qtkit`, and one line in the program's `.pro`:

    QT += dbus                      # optional, and BEFORE the include
    include(vendor/qtkit/qtkit.pri)

The sources compile into the program with its own flags, as softkeys
does, so its `--gc-sections` drops whatever it does not call. Without
`QT += dbus` the portal is left out and the rest works.

Then, after the `QApplication` exists and before the first window:

    qtkit_follow_desktop(application, "SITU_THEME");

or a `qtkit_theme` held by the program where it offers a System / Light
/ Dark choice.

## The theme {#theme}

Status: settled 2026-11-20
Decides: dark-desktop-detection

`include/qtkit/theme.h`. Whether the desktop is dark, qtkit's dark
palette, and following the desktop when it changes. The consult order is
the one `qt-desktop-integration` settles -- the program's choice, an
override variable, Qt's hint, the portal, the desktop's files, the
palette last -- and the header says why.

### Where each part came from {#sources}

Five implementations were read in full on 2026-11-20. What was taken,
and why it beat the others:

| Part | Taken from | Why, and what the others did |
|---|---|---|
| Consult order, 0 and -1 from the portal both falling through | hydra, src/theme.cpp | the settled order; snurra and situ skipped the portal |
| Portal read behind `DBUS_SESSION_BUS_ADDRESS` | hydra, src/session_bus.h; beerssh | asking with no bus autolaunches an unreaped `dbus-daemon` |
| Desktop files read session first | snurra, situ | measured on a machine with TDE and LXQt both installed; hydra read the first file present |
| `~/.config/kdeglobals` for KDE | hydra | snurra and situ read only TDE's and KDE 3's |
| One reader for both dialects, sections respected | hydra `color_scheme_from` | a hex in kdeglobals or a triple in lxqt.conf is still read |
| Channels range-checked | situ | hydra accepted 300 |
| Identical colours abstain | hydra | snurra and situ called them light |
| Dark palette | hydra | the only one setting the five shading roles (Light ... Shadow) and a disabled Highlight; in hydra their absence drew near-white frames under near-white text |
| BrightText in the dark palette | beerssh, snurra, situ | hydra left it at the light value |
| Fusion when dark | snurra, situ | a native style may ignore the palette |
| Light restored as the style's own palette | hydra | beerssh wrote its own light, which replaces the user's light theme |
| Following changes: Qt's signal and the portal's | hydra, beerssh | snurra and situ applied once at start |
| No re-apply when the answer has not changed | beerssh, hydra | setPalette raises the event a platform may report a change through |
| Telling Qt the scheme (Qt 6.8) | hydra, beerssh | Qt's own dialogs read the hint, not the palette |
| The hint skipped once qtkit has set it | new | see below |

**The hint read back, unverified.** Once a program calls
`QStyleHints::setColorScheme`, `colorScheme()` may report that request
rather than the desktop, and a detector asking the hint first would then
never see the desktop change. qtkit skips the hint after setting it. On
this machine the question cannot be put: the offscreen platform and an
unthemed xcb ignore the request, so the test prints that the guard went
unexercised rather than passing silently. hydra and beerssh both set
and then read the hint; whether they stick on a desktop whose platform
theme honours the request (GTK's, Plasma's) is theirs to measure, and it
is recorded here, not reported as a fault.

## Next

Status: open
Decides: qtkit-next

Each when a second tree needs it, from the survey of 2026-11-20:

- **An About window with the copyright line** -- hydra, beerssh and
  fuzzypickles each wrote one; `project-attribution` says what it holds.
- **The desktop's fixed font** for bytes and offsets.
- **`flow_layout`**, written separately by hydra and fuzzypickles.

Moving a program onto qtkit is that program's own work, done when its
session next touches the code it replaces; qtkit does not edit them.

## Working on this

`make` builds `libqtkit.a` so the tree compiles alone; `make test` runs
the suite offscreen with no session bus named; `make style` is the gate.
The suite was proved able to fail by reverting the session-first order,
which it caught.

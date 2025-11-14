## Description

This FAR Manager 3.0 plugin shows metadata for FictionBook files (.FB2) in file panel columns.

The plugin scans for files with extension `.FB2` and `.ZIP`.

Supported column names:

- `title`
- `first-name`
- `last-name`
- `genre`
- `date`
- `sequence`
- `number`

## Installation

Unpack the archive into a subfolder in FAR Manager `Plugins` directory.
The plugin requires Microsoft Visual C++ 2015 Redistributable package installed.

## Configuration

Open *Options* → *File panel modes* menu, select an existing mode or create a new one, modify *Column types* value by adding some column names in angle brackets, add the widths of the columns in *Column widths* field.

```
╔═══════════════════════════ FictionBook ════════════════════════════╗
║ Name                                                               ║
║ FictionBook                                                        ║
║ Column types                     Status line column types          ║
║ N,S,D,T,<title>,<first-name>     N                                 ║
║ Column widths                    Status line column widths         ║
║ 0,8,0,5,25,10                    0                                 ║
╟────────────────────────────────────────────────────────────────────╢
║ [x] Fullscreen view                                                ║
║ [ ] Align file extensions                                          ║
║ [ ] Align folder extensions                                        ║
║ [ ] Show folders in uppercase                                      ║
║ [ ] Show files in lowercase                                        ║
║ [ ] Show uppercase file names in lowercase                         ║
╟────────────────────────────────────────────────────────────────────╢
║                    { OK } [ Reset ] [ Cancel ]                     ║
╚════════════════════════════════════════════════════════════════════╝
```
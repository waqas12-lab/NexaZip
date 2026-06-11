# NexaZip

Modern native WinRAR-style ZIP archive manager built with C++20, Qt6 Widgets, CMake, and zlib.

## Screenshots

![Dashboard](screenshots/dashboard.png)

![Open Archive](screenshots/open_archive.png)

![Extract ZIP](screenshots/extract_zip.png)

## Features

- Native Windows/macOS/Linux title bar
- No custom duplicate window buttons
- ZIP create/extract with Deflate support
- Add files and folders
- Folder preview shows internal files clearly
- Open ZIP and add new files/folders without closing ZIP
- Press Create to update opened ZIP
- Remove selected staged items
- Dark/Light mode
- Search
- CRC test
- Drag and drop
- GitHub ready

## Build on macOS

```bash
brew install cmake qt

cd ~/Downloads
rm -rf NexaZip
unzip NexaZip_stable_theme_folder_fixed2.zip
cd NexaZip
rm -rf build
mkdir build
cd build
cmake .. -DCMAKE_PREFIX_PATH=$(brew --prefix qt)
cmake --build . -j8
open NexaZip.app
```

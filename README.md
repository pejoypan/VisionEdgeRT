# High Performance Machine Vision Architecture

## How to Build

In administrative cmd:
```bash
set_local_vars.bat
mkdir build && cd build
cmake ..
cmake --build . --config release
```
### Build Options

| Features     | Options                 |
| -------- | -------------------- |
| Disable Timer | VERT_DISABLE_TIMING |
| Image Window for Debug | VERT_DEBUG_WINDOW  |
|          |                      |



## How to Run

In powershell prompt
```powershell
cmake --install .
set_local_path.ps1
cd CMAKE_INSTALL_PREFIX
./VERT.exe
```
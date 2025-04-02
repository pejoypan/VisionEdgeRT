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

| 功能     | 参数                 |
| -------- | -------------------- |
| 禁用计时 | VERT_DISABLE_TIMING |
|          |                      |
|          |                      |



## How to Run

In powershell prompt
```powershell
set_local_path.ps1
./build/nodes/<your_favorite_node>/Release/<your_favorite_node>.exe
```
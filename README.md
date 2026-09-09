# ScopeHTTP

ScopeHTTP is a small Windows HTTP server built with the [Crow](https://crowcpp.org/) C++ micro-framework. It listens on `127.0.0.1:8080`, serves `public/index.html` at `/`, serves other assets under `/static/`, and exposes a JSON health endpoint at `/api/status`.

## Prerequisites

- Windows 10 or later
- Visual Studio 2026 (MSBuild 18) with the **Desktop development with C++** workload
- CMake 4.2 or later
- Git
- vcpkg

## Install vcpkg and Crow

Open **Developer PowerShell for VS 2026** and run:

```powershell
cd C:\src
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat
```

The included `vcpkg.json` declares Crow, so this project uses vcpkg manifest mode. From the project directory, install the declared dependencies with:

```powershell
vcpkg install --triplet x64-windows
```

Do not append `crow` to that command; individual package arguments are not accepted in manifest mode. vcpkg will install Crow and its Asio dependency from `vcpkg.json`.

The manifest includes a `builtin-baseline` so vcpkg can resolve versions reproducibly. If your local vcpkg checkout is older than that baseline, update the manifest from the checkout you are using:

```powershell
vcpkg x-update-baseline --add-initial-baseline
vcpkg install --triplet x64-windows
```

## Configure and build

From the ScopeHTTP project directory:

```powershell
$env:VCPKG_ROOT = "C:\src\vcpkg"
cmake --preset windows-vcpkg
cmake --build build-vs18 --config Release
```

If you do not use a preset, the equivalent commands are:

```powershell
cmake -S . -B build-vs18 -G "Visual Studio 18 2026" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build-vs18 --config Release
```

The post-build step copies `public` next to the executable. Run it from the build output directory:

```powershell
.\build-vs18\Release\ScopeHTTP.exe
```

Open <http://127.0.0.1:8080/> or query the API:

```powershell
Invoke-RestMethod http://127.0.0.1:8080/api/status
```

To use another asset directory, set `SCOPEHTTP_PUBLIC_DIR` before starting the process. Static paths are constrained to that directory and directory listings are not enabled.

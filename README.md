# NZ5 Flight Data Visualizer (C++17 / Qt 6 / OpenGL)

A CMake-based C++17 desktop application that loads flight telemetry (`Book1.csv`), sorts AM/PM timestamps chronologically, exports GeoJSON and 15-byte MSB-first binary files concurrently via `std::async`, and renders synchronized 2D and 3D OpenGL views textured from `World_10M.mbtiles`.

## Features (Tasks 1–8)
- **CSV Ingestion & `QTableView`**: Nullable fields handled with `std::optional`.
- **Chronological AM/PM Sort**: Converts 12-hour strings to 24-hour `QTime`.
- **GeoJSON Export**: Sorted waypoints with ISO-8601 timestamps (`outputs/flight_sorted.geojson`).
- **Concurrent Binary Export**: 15-byte `#pragma pack(1)` Big-Endian records written via `std::async` (`3,000 bytes` each).
- **Synchronized 2D Map & 3D Globe**: Web Mercator tile rendering from `World_10M.mbtiles`, yellow flight trajectory, and red SLERP geodesic shortest path.

## Video Walkthrough & Full Package
- **Demo & Code Walkthrough Video (Google Drive):** [(https://drive.google.com/drive/folders/1l-dAnU6NSpABLVdmUqSJ7JfTE7u7ltv5?usp=sharing)]

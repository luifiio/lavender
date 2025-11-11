# lavender

A macOS desktop application for intelligent music library management, featuring audio fingerprinting, automatic metadata retrieval, and ML-powered music recommendations.

## features

- **audio Fingerprinting**: Automatically identify songs using Chromaprint and AcoustID API
- **metadata Retrieval**: Fetch missing song information from MusicBrainz
- **library Scanner**: Recursively scan music directories and extract metadata with TagLib
- **smart Recommendations**: ML-powered recommendation engine using TF-IDF and cosine similarity
- **sqllite Database**: Efficient local storage for library management

### ext libs

- CMake 3.15+
- Qt 6.x
- SQLite3
- TagLib
- FFmpeg
- Chromaprint (fpcalc)
- Python 3.x with scikit-learn and pandas

### build 

```bash
mkdir build && cd build
cmake ..
make
```

macos app bundle will be created in `build/lavender.app`.


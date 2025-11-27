# Minimal Integration Example

This is a minimal working example showing how to integrate IOC_GaiaLib into your application.

## Directory Structure

```
minimal_example/
├── CMakeLists.txt
├── main.cpp
└── README.md (this file)
```

## Files

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(MinimalGaiaApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find IOC_GaiaLib (assumes library is installed system-wide)
find_package(IOC_GaiaLib REQUIRED)

# Create executable
add_executable(minimal_app main.cpp)

# Link library
target_link_libraries(minimal_app PRIVATE IOC_GaiaLib::ioc_gaialib)
```

### main.cpp

```cpp
#include <ioc_gaialib/gaia_mag18_catalog_v2.h>
#include <iostream>
#include <iomanip>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <catalog_path.mag18v2>" << std::endl;
        std::cerr << "Example: " << argv[0] << " gaia_mag18_v2.mag18v2" << std::endl;
        return 1;
    }
    
    std::string catalog_path = argv[1];
    
    try {
        // Open catalog
        std::cout << "Opening catalog: " << catalog_path << std::endl;
        ioc::gaia::Mag18CatalogV2 catalog(catalog_path);
        
        // Enable parallel processing
        catalog.setParallelProcessing(true, 4);
        
        std::cout << "Catalog opened successfully!" << std::endl;
        std::cout << "Total stars: " << catalog.getTotalStars() << std::endl;
        std::cout << "HEALPix NSIDE: " << catalog.getHEALPixNside() << std::endl;
        std::cout << std::endl;
        
        // Example query: 5° cone around M42 (Orion Nebula)
        double ra = 83.8220;   // RA of M42
        double dec = -5.3911;  // Dec of M42
        double radius = 5.0;   // 5 degrees
        
        std::cout << "Querying 5° cone around M42 (RA=" << ra 
                  << ", Dec=" << dec << ")..." << std::endl;
        
        auto stars = catalog.queryCone(ra, dec, radius);
        
        std::cout << "Found " << stars.size() << " stars" << std::endl;
        std::cout << std::endl;
        
        // Print first 10 stars
        std::cout << "First 10 stars:" << std::endl;
        std::cout << std::fixed << std::setprecision(6);
        std::cout << std::string(80, '-') << std::endl;
        
        for (size_t i = 0; i < std::min(stars.size(), size_t(10)); ++i) {
            const auto& star = stars[i];
            std::cout << "Star " << (i+1) << ":" << std::endl;
            std::cout << "  Source ID: " << star.source_id << std::endl;
            std::cout << "  RA:        " << std::setw(10) << star.ra << "°" << std::endl;
            std::cout << "  Dec:       " << std::setw(10) << star.dec << "°" << std::endl;
            std::cout << "  G mag:     " << std::setw(8) << star.phot_g_mean_mag << std::endl;
            std::cout << "  Parallax:  " << std::setw(8) << star.parallax << " mas" << std::endl;
            std::cout << std::endl;
        }
        
        // Query brightest stars
        std::cout << "Brightest 5 stars in cone:" << std::endl;
        std::cout << std::string(80, '-') << std::endl;
        
        auto bright = catalog.queryBrightest(ra, dec, radius, 5);
        
        for (size_t i = 0; i < bright.size(); ++i) {
            const auto& star = bright[i];
            std::cout << (i+1) << ". ID=" << star.source_id 
                      << " | G=" << std::setw(6) << star.phot_g_mean_mag
                      << " | RA=" << std::setw(10) << star.ra
                      << " | Dec=" << std::setw(10) << star.dec << std::endl;
        }
        
    } catch (const std::runtime_error& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "\nMake sure:" << std::endl;
        std::cerr << "  1. The catalog file exists" << std::endl;
        std::cerr << "  2. You have read permissions" << std::endl;
        std::cerr << "  3. The file is a valid .mag18v2 catalog" << std::endl;
        return 1;
        
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
```

## Build Instructions

### Option 1: System-Installed Library

If you installed IOC_GaiaLib system-wide:

```bash
mkdir build && cd build
cmake ..
make
./minimal_app /path/to/gaia_mag18_v2.mag18v2
```

### Option 2: Custom Install Location

If you installed to a custom prefix:

```bash
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=/path/to/install ..
make
./minimal_app /path/to/gaia_mag18_v2.mag18v2
```

### Option 3: Without Installing

Point to the build directory:

```cmake
# In CMakeLists.txt, replace find_package with:
set(IOC_GaiaLib_DIR "/path/to/IOC_GaiaLib/build")
include_directories(${IOC_GaiaLib_DIR}/../include)
link_directories(${IOC_GaiaLib_DIR})

target_link_libraries(minimal_app PRIVATE 
    ioc_gaialib 
    pthread z curl xml2 omp
)
```

## Expected Output

```
Opening catalog: gaia_mag18_v2.mag18v2
Catalog opened successfully!
Total stars: 303085895
HEALPix NSIDE: 64

Querying 5° cone around M42 (RA=83.822, Dec=-5.3911)...
Found 42305 stars

First 10 stars:
--------------------------------------------------------------------------------
Star 1:
  Source ID: 3018395321567891456
  RA:        83.456789°
  Dec:       -5.234567°
  G mag:     12.345678
  Parallax:   0.123456 mas

[...]

Brightest 5 stars in cone:
--------------------------------------------------------------------------------
1. ID=3018395321567891456 | G=  8.123 | RA= 83.456789 | Dec= -5.234567
2. ID=3018395321567891457 | G=  8.456 | RA= 83.567890 | Dec= -5.345678
[...]
```

## Troubleshooting

### "Could NOT find IOC_GaiaLib"

Install the library first:
```bash
cd /path/to/IOC_GaiaLib
mkdir build && cd build
cmake ..
make
sudo make install
```

### "Catalog file not found"

Download the catalog:
```bash
# See docs/CATALOG_DOWNLOADS.md for download instructions
```

Or use absolute path:
```bash
./minimal_app /full/path/to/gaia_mag18_v2.mag18v2
```

### Linking errors

Make sure all dependencies are installed:
```bash
# macOS
brew install curl libxml2 zlib libomp

# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev libxml2-dev zlib1g-dev libomp-dev
```

## Next Steps

- See [docs/INTEGRATION_GUIDE.md](../docs/INTEGRATION_GUIDE.md) for complete integration guide
- See [docs/GAIA_MAG18_CATALOG_V2.md](../docs/GAIA_MAG18_CATALOG_V2.md) for V2 catalog details
- See [examples/](../examples/) for more complex examples

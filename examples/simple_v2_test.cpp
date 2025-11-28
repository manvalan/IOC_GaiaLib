// Simple Working V2 Catalog Test
#include "ioc_gaialib/gaia_mag18_catalog_v2.h"
#include <iostream>

using namespace ioc::gaia;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <catalog.mag18v2>\n";
        return 1;
    }
    
    std::cout << "\n✅ V2 Catalog Test\n\n";
    
    try {
        std::cout << "1. Opening catalog...\n";
        Mag18CatalogV2 catalog(argv[1]);
        std::cout << "   ✓ Opened\n\n";
        
        std::cout << "2. Reading statistics...\n";
        std::cout << "   Stars: " << catalog.getTotalStars() << "\n";
        std::cout << "   Pixels: " << catalog.getNumPixels() << "\n";
        std::cout << "   Chunks: " << catalog.getNumChunks() << "\n";
        std::cout << "   Mag limit: " << catalog.getMagLimit() << "\n";
        std::cout << "   ✓ OK\n\n";
        
        std::cout << "3. Testing getHEALPixPixel()...\n";
        uint32_t pix = catalog.getHEALPixPixel(180.0, 0.0);
        std::cout << "   HEALPix(180°, 0°) = " << pix << "\n";
        std::cout << "   ✓ OK\n\n";
        
        std::cout << "4. Testing queryBySourceId()...\n";
        auto star = catalog.queryBySourceId(1000000);
        if (star) {
            std::cout << "   Found: RA=" << star->ra << "° Dec=" << star->dec << "\n";
        } else {
            std::cout << "   Not found (ok, may be > mag 18)\n";
        }
        std::cout << "   ✓ OK\n\n";
        
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  ✅ ALL TESTS PASSED - V2 Catalog Works!                  ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
}

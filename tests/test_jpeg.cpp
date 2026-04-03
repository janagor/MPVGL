/**
 *  Your Project
 *
 *  Copyright (C) 2026 Jan Aleksander Górski
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty. In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *  1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would
 *     be appreciated but is not required.
 *
 *  2. Altered source versions must be plainly marked as such, and must not
 *     be misrepresented as being the original software.
 *
 *  3. This notice may not be removed or altered from any source
 *     distribution.
 */

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "MPVGL/IO/JPEG/JPEG.hpp"

int main(int argc, char** argv) {
    std::string inputPath = "test.jpg";
    if (argc > 1) {
        inputPath = argv[1];
    }

    std::cout << "[Test] Proba wczytania: " << inputPath << "...\n";

    std::ifstream file(inputPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "[BLAD] Nie mozna otworzyc pliku: " << inputPath << "!\n";
        return 1;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<std::byte> fileData(size);
    if (!file.read(reinterpret_cast<char*>(fileData.data()), size)) {
        std::cerr
            << "[BLAD] Nie udalo sie wczytac zawartosci pliku do pamieci!\n";
        return 1;
    }
    file.close();
    std::cout << "[INFO] Wczytano " << size << " bajtow do pamieci.\n";

    auto result = mpvgl::jpeg::JPEGLoader::load(fileData);

    if (!result.has_value()) {
        std::cerr << "[BLAD] Dekodowanie JPEG nie powiodlo sie!\n";
        std::cerr << "Kod bledu: " << result.error().code().value() << "\n";
        std::cerr << "Wiadomosc: " << result.error().message() << "\n";
        return 1;
    }

    auto image = std::move(result.value());
    auto view = image.view();

    std::cout << "[SUKCES] Obraz zdekodowany poprawnie!\n";
    std::cout << "Szerokosc: " << view.width() << " px\n";
    std::cout << "Wysokosc:  " << view.height() << " px\n";

    std::string outputPath = "output.ppm";
    std::ofstream outFile(outputPath, std::ios::binary);

    if (!outFile) {
        std::cerr << "[BLAD] Nie mozna otworzyc pliku " << outputPath
                  << " do zapisu!\n";
        return 1;
    }

    outFile << "P6\n" << view.width() << " " << view.height() << "\n255\n";

    auto rawBytes = image.bytes();
    outFile.write(reinterpret_cast<char const*>(rawBytes.data()),
                  rawBytes.size_bytes());
    outFile.close();

    std::cout << "[SUKCES] Zapisano zdekodowany obraz do pliku: " << outputPath
              << "\n";
    std::cout << "Otworz plik 'output.ppm' w przegladarce (np. GIMP, "
                 "IrfanView), aby zobaczyc wynik.\n";

    return 0;
}

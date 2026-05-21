#include <iostream>
#include <string>
#include <limits>
#include "../src/SubstringFrequency.h"

void RunAutoMode() {
    std::cout << "[*] Generating massive test file (1 000 000 chars)..." << std::endl;

    FileOutputStream out("load_test.txt");
    const std::string chunk = "ababacabad";
    for(int i = 0; i < 100000; ++i) {
        for(char c : chunk) {
            out.Output(c);
        }
    }
    out.Close();

    std::string patterns[] = {"abacaba", "bad", "caba", "xyz"};
    AhoCorasick ac(patterns, 4);

    FileInputStream fstream("load_test.txt");

    std::cout << "[*] Running Aho-Corasick over 1M char stream in O(N)..." << std::endl;
    int* freqs = ac.ProcessStream(&fstream);

    std::cout << "--- Frequency Results ---" << std::endl;
    for (int i = 0; i < 4; ++i) {
        std::cout << "Pattern '" << patterns[i] << "': " << freqs[i] << " times." << std::endl;
    }
    delete[] freqs;
}

void RunManualMode() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    std::cout << "Enter text to search in (can include spaces): ";
    std::string text;
    std::getline(std::cin, text);

    std::cout << "Enter pattern to search (can include spaces): ";
    std::string pattern;
    std::getline(std::cin, pattern);

    std::string patterns[] = {pattern};
    AhoCorasick ac(patterns, 1);

    SequenceOutputStream<char> outStream;
    for(char c : text) {
        outStream.Output(c);
    }

    Sequence<char>* seq = outStream.ReleaseSequence(); // Забрали у потока записи, отдали потоку чтения
    SequenceInputStream<char> stream(seq);

    int* freqs = ac.ProcessStream(&stream);
    std::cout << "Frequency of '" << pattern << "': " << freqs[0] << std::endl;

    delete[] freqs;
    delete seq;
}

void StartUI() {
    while (true) {
        std::cout << "\n===== SEQUENCE & ALGORITHMS MENU =====\n";
        std::cout << "1. Manual Mode (Input custom string)\n";
        std::cout << "2. Auto Mode (Load test 1M characters stream)\n";
        std::cout << "3. Exit\n";
        std::cout << "Choice: ";

        int choice;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (choice == 1) RunManualMode();
        else if (choice == 2) RunAutoMode();
        else if (choice == 3) break;
        else std::cout << "Invalid choice.\n";
    }
}

#include <iostream>
#include <string>
#include <fstream>
#include "../src/SubstringFrequency.h"
#include "../src/Stream.h"
#include "ArraySequence.h"

void RunAutoMode() {
    std::cout << "[*] Generating massive test file (1 000 000 chars)..." << std::endl;
    std::ofstream out("load_test.txt");
    for(int i = 0; i < 100000; ++i) out << "ababacabad";
    out.close();

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
    std::cout << "Enter text to search in: ";
    std::string text;
    std::cin >> text;

    std::cout << "Enter pattern to search: ";
    std::string pattern;
    std::cin >> pattern;

    std::string patterns[] = {pattern};
    AhoCorasick ac(patterns, 1);

    Sequence<char>* seq = new MutableArraySequence<char>();
    for(char c : text) appendTracked(seq, c);
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
        std::cin >> choice;

        if (choice == 1) RunManualMode();
        else if (choice == 2) RunAutoMode();
        else if (choice == 3) break;
        else std::cout << "Invalid choice.\n";
    }
}

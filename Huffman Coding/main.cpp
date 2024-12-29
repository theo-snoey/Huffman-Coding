#include <iostream>
#include "bits.h"
#include "console.h"
#include "filelib.h"
#include "huffman.h"
#include "simpio.h"
#include "strlib.h"
#include "SimpleTest.h"
using namespace std;


void huffmanConsoleProgram();

/*
 * You are free to edit the main in any way that works
 * for your testing/debugging purposes.
 * We will supply our own main() during grading.
 */
int main() {
    if (runSimpleTests(SELECTED_TESTS)) {
        return 0;
    }

    /*
     * In order to run the console program to compress/decompress whole files
     * respond 0 when asked for which tests, and this falls through
     * to the code below.
     */
    huffmanConsoleProgram();

    cout << endl << "main() completed." << endl;
    return 0;
}


/*
 * Sets up the output console and explains the program to the user.
 */
void intro() {
    cout << "Welcome to CS106B Shrink-It!" << endl;
    cout << "This program uses the Huffman coding algorithm for compression." << endl;
    cout << "Any type of file can be encoded using a Huffman code." << endl;
    cout << "Decompressing the result will faithfully reproduce the original." << endl;
}

/*
 * Prints a menu of choices for the user and reads/returns the user's response.
 */
string menu() {
    cout << endl;
    cout << "Your options are:" << endl;
    cout << "C) compress file" << endl;
    cout << "D) decompress file" << endl;
    cout << "Q) quit" << endl;

    cout << endl;
    return toUpperCase(trim(getLine("Enter your choice: ")));
}

/*
 * Allow user to choose filename for input (output name will be parallel).
 */
bool getInputAndOutputFiles(string& inFilename, string& outFilename, bool compressing) {
    const string kCompressedExtension = ".huf";
    const string kDecompressedExtension = "unhuf.";

    if (compressing) {
        inFilename = chooseFilenameDialog("Choose file to compress", "res/");
    } else {
        inFilename = chooseFilenameDialog("Choose file to decompress", "res/", "*" + kCompressedExtension);
    }
    if (inFilename.empty()) {
        cout << "Operation canceled. " << endl;
        return false;
    }
    cout << "Reading file: " << inFilename << endl;
    if (compressing) {
        outFilename = inFilename + kCompressedExtension;
    } else {
        string head = getHead(inFilename);
        string tail = getTail(inFilename);
        outFilename = (!head.empty()? head + "/" : "") + kDecompressedExtension + getRoot(tail);
    }
    cout << "Writing file: " << outFilename << endl;
    if (fileExists(outFilename) && !getYesOrNo("File already exists. Overwrite? (y/n) ")) {
        cout << "Operation canceled. " << endl;
        return false;
    }
    return true;
}

/*
 * Compress a file.
 * Prompts for input/output file names and opens streams on those files.
 * Then calls your compress function and displays information about size of
 * compressed output.
 */
void compressFile() {
    string inFilename, outFilename;

    if (!getInputAndOutputFiles(inFilename, outFilename, true)) {
        return;
    }
    cout << "Reading " << fileSize(inFilename) << " bytes from " << inFilename << endl;
    try {
        string text = readRawBinary(inFilename);
        cout << "Compressing ..." << endl;
        EncodedData data = compress(text);
        writeEncodedData(data, outFilename);
        cout << "Wrote " << fileSize(outFilename) << " compressed bytes to " << outFilename << endl;
    } catch (ErrorException& e) {
        cerr << "Unable to write compressed file: " << e.getMessage() << endl;
    }
}

/*
 * Decompress a file.
 * Prompts for input/output file names and opens streams on those files.
 * Then calls your decompress function and displays information about size of
 * decompressed output.
 */
void decompressFile() {
    string inFilename, outFilename;

    if (!getInputAndOutputFiles(inFilename, outFilename, false)) {
        return;
    }
    cout << "Reading " << fileSize(inFilename) << " bytes from " << inFilename << endl;
    try {
        EncodedData data = readEncodedData(inFilename);
        cout << "Decompressing ..." << endl;
        string text = decompress(data);
        writeRawBinary(text, outFilename);
        cout << "Wrote " << fileSize(outFilename) << " decompressed bytes to " << outFilename << endl;
    } catch (ErrorException& e) {
        cerr << "Unable to decompress: " << e.getMessage() << endl;
    }
}

void huffmanConsoleProgram() {
    intro();
    while (true) {
        string choice = menu();
        if (choice == "Q") {
            break;
        } else if (choice == "C") {
            compressFile();
        } else if (choice == "D") {
            decompressFile();
        }
    }
}


// Do not remove or edit below this line. It is here to confirm that your code
// conforms to the expected function prototypes needed for grading
void confirmFunctionPrototypes() {
    string str;
    EncodingTreeNode *t;
    Queue<Bit> qb;
    Queue<char> qc;
    EncodedData data;

    t = buildHuffmanTree(str);
    deallocateTree(t);
    str = decodeText(t, qb);
    qb = encodeText(t, str);
    flattenTree(t, qb, qc);
    t = unflattenTree(qb, qc);
    data = compress(str);
    str =  decompress(data);
}

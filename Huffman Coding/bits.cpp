#include "bits.h"
#include "error.h"
#include <string>
#include <vector>
#include <fstream>
using namespace std;

/**
 * Routines for managing Bit class and for reading/write Bits within EncodedData
 * objects to a stream.  The public interface is provided in bits.h header file.
 * You do not need to revew the implementation details in this file but are welcome
 * to take a peek if you're curious.
 */

Bit::Bit(int value) {
    /* Check for use of chararacter values. */
    if (value == '0' || value == '1') {
        error("You have attempted to create a bit equal to the character '0' or '1'. "
              "The characters '0' and '1' are not the same as the numbers 0 and 1. "
              "Edit your code to instead use the numeric values 0 and 1 instead.");
    }
    if (value != 0 && value != 1) {
        error("Illegal value for a bit: " + to_string(value));
    }

    _value = (value == 1);
}

bool operator== (Bit lhs, Bit rhs) {
    return lhs._value == rhs._value;
}
bool operator!= (Bit lhs, Bit rhs) {
    return !(lhs == rhs);
}
ostream& operator<< (ostream& out, Bit bit) {
    return out << (bit._value? '1' : '0');
}


namespace {
    /**
     * Validates that the given EncodedData obeys all the invariants we expect it to.
     */
    void checkIntegrityOf(const EncodedData& data) {
        /* Number of distinct characters must be at least two. */
        if (data.treeLeaves.size() < 2) {
            error("EncodedData is invalid (flattened encoding tree does not contain at least two leaf nodes)");
        }

        /* Number of bits in tree shape should be exactly 2c - 1, where c is the number of
         * distinct characters.
         */
        if (data.treeShape.size() != data.treeLeaves.size() * 2 - 1) {
            error("EncodedData is invalid (flattened encoding tree has mismatch in counts of tree shape bits and tree leaves)");
        }
    }

    /* Utility types for reading/writing individual bits. Inspired by a
     * similar implementation by Julie Zelenski.
     */
    class BitWriter {
    public:
        explicit BitWriter(ostream& o) : _out(o) {}
        ~BitWriter() {
            if (_bitIndex != 0) flush();
        }

        void put(Bit b) {
            if (b != 0) {
                _bitBuffer |= (1U << _bitIndex);
            }

            _bitIndex++;
            if (_bitIndex == 8) {
                flush();
            }
        }

    private:
        void flush() {
            _out.put(_bitBuffer);
            _bitBuffer = 0;
            _bitIndex = 0;
        }

        ostream& _out;
        uint8_t _bitBuffer = 0;
        uint8_t _bitIndex  = 0;
    };

    class BitReader {
    public:
        explicit BitReader(istream& i) : _in(i) {}

        Bit get() {
            if (_bitIndex == 8) readMore();

            Bit result = !!(_bitBuffer & (1U << _bitIndex));
            _bitIndex++;
            return result;
        }

    private:
        istream& _in;
        uint8_t _bitBuffer = 0;
        uint8_t _bitIndex  = 8;

        void readMore() {
            char read;
            if (!_in.get(read)) {
                error("Unexpected end of file when reading bits.");
            }

            _bitBuffer = read;
            _bitIndex = 0;
        }
    };

    /* "CS106B A7" */
    const uint32_t kFileHeader = 0xC5106BA7;
}

/**
 * We store EncodedData on disk as follows:
 *
 *
 * 1 byte:  number of distinct characters, minus one.
 * c bytes: the leaves of the tree, in order.
 * 1 byte:  number of valid bits in the last byte.
 * n bits:  tree bits, followed by message bits.
 *
 * We don't need to store how many bits are in the tree, since it's always given
 * by 2*c - 1, as this is the number of nodes in a full binary tree with c leaves.
 */
void writeEncodedData(EncodedData& data, const string& filename) {
    ofstream out(filename, ios::binary);
    if (out.fail()) {
        error("Error writeEncodedData(" + filename + "): cannot open file for writing.");
    }

    /* Validate invariants. */
    checkIntegrityOf(data);

    /* Write magic header. */
    out.write(reinterpret_cast<const char *>(&kFileHeader), sizeof kFileHeader);

    /* Number of characters. */
    const uint8_t charByte = data.treeLeaves.size() - 1;
    out.put(charByte);

    /* Tree leaves. */
    while (!data.treeLeaves.isEmpty()) out.put(data.treeLeaves.dequeue());

    /* Number of bits in the last byte to read. */
    uint8_t modulus = (data.treeShape.size() + data.messageBits.size()) % 8;
    if (modulus == 0) modulus = 8;
    out.put(modulus);

    /* Bits themselves. */
    BitWriter writer(out);
    while (!data.treeShape.isEmpty()) writer.put(data.treeShape.dequeue());
    while (!data.messageBits.isEmpty()) writer.put(data.messageBits.dequeue());
}

/**
 * Reads EncodedData from stream.
 */
EncodedData readEncodedData(const string& filename) {
    ifstream in(filename, ios::binary);
    if (in.fail()) {
        error("Error readEncodedData(" + filename + "): cannot open file for reading.");
    }

    /* Read back the magic header and make sure it matches. */
    uint32_t header;
    if (!in.read(reinterpret_cast<char *>(&header), sizeof header) ||
        header != kFileHeader) {
        error("Error readEncodedData(" + filename + "): does not start with Huffman-compressed header.");
    }

    EncodedData data;

    /* Read the character count. */
    char skewCharCount;
    if (!in.get(skewCharCount)) {
        error("Error readEncodedData(" + filename + "): unable to read character count.");
    }

    /* We offset this by one - add the one back. */
    int charCount = uint8_t(skewCharCount);
    charCount++;

    if (charCount < 2) {
        error("Error readEncodedData(" + filename + "): character count too low.");
    }

    /* Read in the leaves. */
    vector<char> leaves(charCount);
    if (!in.read(leaves.data(), leaves.size())) {
        error("Error readEncodedData(" + filename + "): unable to read all tree leaves.");
    }
    for (char leaf: leaves) {
        data.treeLeaves.enqueue(leaf);
    }

    /* Read in the modulus. */
    char signedModulus;
    if (!in.get(signedModulus)) {
        error("Error readEncodedData(" + filename + "): unable to read modulus.");
    }
    uint8_t modulus = signedModulus;

    /* See how many bits we need to read. To do this, jump to the end of the file
     * and back to where we are to count the bytes, then transform that to a number
     * of bits.
     *
     * Thanks to Julie Zelenski for coming up with this technique!
     */
    auto currPos = in.tellg();
    if (!in.seekg(0, istream::end)) {
        error("Error readEncodedData(" + filename + "): cannot seek to end of file.");
    }
    auto endPos  = in.tellg();
    if (!in.seekg(currPos, istream::beg)) {
        error("Error readEncodedData(" + filename + "): cannot seek to middle of file.");
    }

    /* Number of bits to read = (#bytes - 1) * 8 + modulus. */
    uint64_t bitsToRead = (endPos - currPos - 1) * 8 + modulus;

    /* Read in the tree shape bits. */
    BitReader reader(in);
    for (int i = 0; i < 2 * charCount - 1; i++) {
        data.treeShape.enqueue(reader.get());
        bitsToRead--;
    }

    /* Read in the message bits. */
    while (bitsToRead > 0) {
        data.messageBits.enqueue(reader.get());
        bitsToRead--;
    }

    return data;
}

/* These routines open the stream in binary mode to be sure we read/write
 * raw bytes as-is. Without this, file is opened in text mode by default and
 * conversion shenanigans may be present. */

void writeRawBinary(const string& data, const string& filename) {
    ofstream out(filename, ios::binary);
    out.write(data.c_str(), data.size());
    if (out.fail()){
        error("Error writeRawBinary(" + filename + "): unable to write file.");
    }
}

std::string readRawBinary(const string& filename) {
    ifstream in(filename, ios::binary);
    std::ostringstream os;
    os << in.rdbuf();
    if (in.fail()) {
        error("Error readRawBinary(" + filename + "): unable to read file.");
    }
    return os.str();
}

/* For debugging purposes. */
ostream& operator<< (ostream& out, const EncodedData& data) {
    ostringstream builder;
    builder << "{treeShape:" << data.treeShape
            << ",treeLeaves:" << data.treeLeaves
            << ",messageBits:" << data.messageBits
            << "}";
    return out << builder.str();
}

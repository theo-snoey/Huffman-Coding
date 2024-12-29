/*
 * This file huffman.cpp, uses huffman encoding/decoding to compress and decompress files.
 * Author: Theo Snoey
 */
#include "bits.h"
#include "treenode.h"
#include "huffman.h"
#include "map.h"
#include "vector.h"
#include "priorityqueue.h"
#include "strlib.h"
#include "SimpleTest.h"
using namespace std;

/**
 * Given a Queue<Bit> containing the compressed message bits and the original
 * encoding tree, decode the bits back to the original message text.
 *
 * You can assume that tree is a well-formed non-empty encoding tree and the
 * messageBits queue contains a valid sequence of encoded bits for that tree.
 *
 * Your implementation may change the messageBits queue however you like. There
 * are no requirements about its contents after this function returns.
 * The encoding tree should be unchanged.
 */
string decodeText(EncodingTreeNode* tree, Queue<Bit>& messageBits) {

    EncodingTreeNode* cur = tree;
    string result = "";

    if (cur == nullptr){
        return "";

    }

    while (messageBits.size() >= 0){
        if (messageBits.size() == 0){
            if (cur->zero == nullptr && cur->one == nullptr){
                result += cur->ch;
            }
            break;
        }

        Bit curDigit = messageBits.peek();

        if (curDigit == 0 && (cur->zero != nullptr)){
            cur = cur->zero;
            messageBits.dequeue();
        }
        else if (curDigit == 1 && (cur->one != nullptr)){
            cur = cur->one;
            messageBits.dequeue();
        }

        else {
            result += cur->ch;
            cur = tree;
        }
    }

    return result;
}



// this function, unflatten tree helper is the recursive functionality for the unflatten tree function, it takes the already
// computed root node, the treeshape queue of bits, and the treeleaves queue of chars. It then goes through the list of shape bits and
// recursively creates its structure.
void unflattenTreeHelper(Queue<Bit>& treeShape, Queue<char>& treeLeaves, EncodingTreeNode*& cur){

    // base case is when there are no more shape bits to deal with
    if (treeShape.isEmpty()){
        return;
    }

    // dequeque the current bit from the treeshape queue
    Bit curDigit = treeShape.dequeue();

    // if 0, then the new tree node is a leaf, we create a leaf with dequeued char in order from the leaves char queue

    if (curDigit == 0){
        cur = new EncodingTreeNode(treeLeaves.dequeue());
    }

    // if 1, then the node has two children, so we create a new node with two children and keep recursively calling on the two children
    if (curDigit == 1){

        cur = new EncodingTreeNode(nullptr, nullptr);

        // start with left to maintain preorder traveral
        unflattenTreeHelper(treeShape, treeLeaves, cur->zero);
        unflattenTreeHelper(treeShape, treeLeaves, cur->one);

    }
}



/**
 * Reconstruct an encoding tree from flattened form Queue<Bit> and Queue<char>.
 *
 * You can assume that the queues are well-formed and the shape/leaves sequences
 * form a valid encoding tree.
 *
 * Your implementation may change the treeShape and treeLeaves queues however you like.
 * There are no requirements about their contents after this function returns.
 */
EncodingTreeNode* unflattenTree(Queue<Bit>& treeShape, Queue<char>& treeLeaves) {

    // set up the root of the tree and call recursive function
    EncodingTreeNode* root = nullptr;

    unflattenTreeHelper(treeShape, treeLeaves, root);

    return root;
}

/**
 * Decompress the given EncodedData and return the original text.
 *
 * You can assume the data argument is well-formed and was created by a correct
 * implementation of compress.
 *
 * Your implementation may change the EncodedData however you like. There
 * are no requirements about its contents after this function returns.
 */
string decompress(EncodedData& data) {

    Queue<Bit> treeShape = data.treeShape;
    Queue<char> treeLeaves = data.treeLeaves;
    Queue<Bit> messageBits = data.messageBits;

    EncodingTreeNode* root = unflattenTree(treeShape, treeLeaves);

    string myDecodedText = decodeText(root, messageBits);
    deallocateTree(root);

    return myDecodedText;
}



// sumFrequencies takes a pointer to the root of a huffman tree and a map of characters to int frequencies of character uses
// and uses recursion to go through a huffman tree and to return the the sum of the frequency values of the leaves of that tree.
int sumFrequencies(EncodingTreeNode* root, Map<char, int> charfrequencies){
    if (root == nullptr){
        return 0;
    }

    if (root->zero == nullptr && root->one == nullptr){
        int frequency = charfrequencies[root->ch];
        return frequency;
    }

    return sumFrequencies(root->zero, charfrequencies) + sumFrequencies(root->one, charfrequencies);
}

/**
 * Constructs an optimal Huffman coding tree for the given text, using
 * the Huffman algorithm.
 *
 * Reports an error if the input text does not contain at least
 * two distinct characters.
 *
 * It can be helpful for you to establish/document the expected behavior of
 * choices re: tie-breaking, choice of which subtree on which side.
 * These choices do not affect correctness or optimality of resulting tree
 * but knowing which is used allows you to construct test cases that depend
 * on that behavior. Our provided test cases expect:
 *  -- our pqueue dequeues elems of equal priority in FIFO order
 *  -- when build new interior node, first elem dequeued placed as zero subtree
 *     and second elem dequeued placed as one subtree
 */
EncodingTreeNode* buildHuffmanTree(string text) {

    toUpperCase(text);
    Map<char, int> charfrequencies;
    PriorityQueue<EncodingTreeNode*> pq;
    EncodingTreeNode *parent = nullptr;

    // calculate frequencies of different characters in text and map them
    for (char thisChar: text){
        charfrequencies[thisChar] ++;
    }

    if (charfrequencies.size() < 2){
        error("Input text must contain at least two distinct characters!");
    }

    // for each char, create a treenode and pass into a priority queue
    for (char myChar: charfrequencies.keys()){
        EncodingTreeNode *thisLeaf = new EncodingTreeNode(myChar);
        pq.enqueue(thisLeaf, charfrequencies[myChar]);
    }

    while (pq.size() > 1){
        EncodingTreeNode *node1 = pq.dequeue();
        EncodingTreeNode *node2 = pq.dequeue();
        EncodingTreeNode *thisParent = new EncodingTreeNode(node1, node2);
        if (pq.isEmpty()){
            parent = thisParent;
            break;
        }
        pq.enqueue(thisParent, sumFrequencies(thisParent, charfrequencies));
    }
    return parent;
}


// this function, codeMapMaker, takes an pointer to an encodingTreeNode, a sequence of current bits, and a map that maps chars
// to queues of bits. It uses these params to recrusively go through the tree and when encountering leaves, maps the leaf character
// to the seqeunce of bits representing the path to get to that leaf.
void codeMapMaker(EncodingTreeNode*& cur, Queue<Bit> curSequence, Map<char, Queue<Bit>>& charToBit){

    if (cur == nullptr){
        return;
    }

    if (cur->zero == nullptr && cur->one == nullptr){
        charToBit[cur->ch] = curSequence;
        return;
    }

    Queue<Bit> curSequence1 = curSequence;
    Queue<Bit> curSequence2 = curSequence;
    curSequence1.enqueue(Bit(0));
    curSequence2.enqueue(Bit(1));

    codeMapMaker(cur->zero, curSequence1, charToBit);
    codeMapMaker(cur->one, curSequence2, charToBit);
}

/**
 * Given a string and an encoding tree, encode the text using the tree
 * and return a Queue<Bit> of the encoded bit sequence.
 *
 * You can assume tree is a well-formed encoding tree and contains
 * an encoding for every character in the text.
 */

Queue<Bit> encodeText(EncodingTreeNode* tree, string text) {
    Queue<Bit> resultCode;
    Queue<Bit> curSequence;

    Map<char, Queue<Bit>> charToBit;

    codeMapMaker(tree, curSequence, charToBit);

    for (char c : text){

        int bitQueueSize = charToBit[c].size();
        for (int i = 0; i < bitQueueSize; i++){
            Bit elem = charToBit[c].dequeue();
            resultCode.enqueue(elem);
            charToBit[c].enqueue(elem);
        }
    }

    return resultCode;
}

/**
 * Flatten the given tree into a Queue<Bit> and Queue<char> in the manner
 * specified in the assignment writeup.
 *
 * You can assume the input queues are empty on entry to this function.
 *
 * You can assume tree is a well-formed encoding tree.
 */
void flattenTree(EncodingTreeNode* tree, Queue<Bit>& treeShape, Queue<char>& treeLeaves) {
    if (tree == nullptr){
        return;
    }

    if (tree->zero == nullptr && tree->one == nullptr){
        treeShape.enqueue(Bit(0));
        treeLeaves.enqueue(tree->ch);
    }
    else{
        treeShape.enqueue(Bit(1));
    }
    flattenTree(tree->zero, treeShape, treeLeaves);
    flattenTree(tree->one, treeShape, treeLeaves);
}

/**
 * Compress the message text using Huffman coding, producing as output
 * an EncodedData containing the encoded message bits and flattened
 * encoding tree.
 *
 * Reports an error if the message text does not contain at least
 * two distinct characters.
 */
EncodedData compress(string messageText) {

    EncodedData encodedMessage;

    Queue<Bit> treeShape;
    Queue<char> treeLeaves;

    EncodingTreeNode* myTree = buildHuffmanTree(messageText);

    flattenTree(myTree, treeShape, treeLeaves);

    Queue<Bit> messageBits = encodeText(myTree, messageText);

    encodedMessage.messageBits = messageBits;
    encodedMessage.treeLeaves = treeLeaves;
    encodedMessage.treeShape = treeShape;

    deallocateTree(myTree);

    return encodedMessage;
}

/* * * * * * Testing Helper Functions Below This Point * * * * * */

EncodingTreeNode* createExampleTree() {
    /* Example encoding tree used in multiple test cases:
     *                *
     *              /   \
     *             T     *
     *                  / \
     *                 *   E
     *                / \
     *               R   S
     */
    /* TODO: Implement this utility function needed for testing. */

    // create leaves
    EncodingTreeNode* leaf1 = new EncodingTreeNode('T');
    EncodingTreeNode* leaf2 = new EncodingTreeNode('R');
    EncodingTreeNode* leaf3 = new EncodingTreeNode('S');
    EncodingTreeNode* leaf4 = new EncodingTreeNode('E');


    // create interior notes and root
    EncodingTreeNode* INode2 = new EncodingTreeNode(leaf2, leaf3);
    EncodingTreeNode* INode1 = new EncodingTreeNode(INode2, leaf4);
    EncodingTreeNode* root = new EncodingTreeNode(leaf1, INode1);

    return root;



}

void deallocateTree(EncodingTreeNode* t) {
    if (t == nullptr){
        return;
    }

    deallocateTree(t->zero);
    deallocateTree(t->one);

    delete t;

}

bool areEqual(EncodingTreeNode* a, EncodingTreeNode* b) {

    if (a == nullptr && b == nullptr){
        return true;
    }

    if (a == nullptr || b == nullptr){
        return false;
    }

    bool result1 = false;
    bool result2 = false;


    // left is not leaf node -> just check for node assignment equivalency
    if (a->zero != nullptr && b->zero != nullptr){
        result1 = areEqual(a->zero, b->zero);
    }

    // left is leaf node -> check for character equivalency before call
    if (a->zero == nullptr && b->zero == nullptr){
        if (a->ch == b->ch){
            result1 = areEqual(a->zero, b->zero);
        }
    }

    // right is not a leaf node -> check for node assignment
    if (a->one != nullptr && b->one != nullptr){
        result2 = areEqual(a->one, b->one);
    }

    // right is a leaf node -> check for character equivalency before call
    if (a->one == nullptr && b->one == nullptr){
        if (a->ch == b->ch){
            result2 = areEqual(a->one, b->one);
        }
    }


    return result1 && result2;

}

/* * * * * * Test Cases Below This Point * * * * * */

//________________________________________STUDENT_TESTS________________________________________________________

STUDENT_TEST("call allocate tree, then deallocate tree"){
    EncodingTreeNode *tree = createExampleTree();
    deallocateTree(tree);
}

STUDENT_TEST("Are equal tests: two empty trees"){
    EncodingTreeNode *tree1 = nullptr;
    EncodingTreeNode *tree2 = nullptr;

    EXPECT(areEqual(tree1, tree2));

    deallocateTree(tree1);
    deallocateTree(tree2);
}

STUDENT_TEST("Are equal tests: one simple, one empty"){
    EncodingTreeNode *leafOne = new EncodingTreeNode('A');
    EncodingTreeNode *leafTwo = new EncodingTreeNode('B');
    EncodingTreeNode *simpleTree = new EncodingTreeNode(leafOne, leafTwo);

    EncodingTreeNode *emptyTree = nullptr;

    EXPECT(!areEqual(simpleTree, emptyTree));

    deallocateTree(simpleTree);
    deallocateTree(emptyTree);
}

STUDENT_TEST("Are equal/ not equal tests: simple equal to simple, simple not equal to simple"){
    EncodingTreeNode *leafOne = new EncodingTreeNode('A');
    EncodingTreeNode *leafTwo = new EncodingTreeNode('B');
    EncodingTreeNode *simpleTree = new EncodingTreeNode(leafOne, leafTwo);

    EncodingTreeNode *leafOne2 = new EncodingTreeNode('A');
    EncodingTreeNode *leafTwo2 = new EncodingTreeNode('B');
    EncodingTreeNode *simpleTree2 = new EncodingTreeNode(leafOne2, leafTwo2);

    EXPECT(areEqual(simpleTree, simpleTree2));


    leafOne2->ch = 'K';
    leafTwo2->ch = 'J';


    EXPECT(!areEqual(simpleTree, simpleTree2));

    deallocateTree(simpleTree);
    deallocateTree(simpleTree2);
}

STUDENT_TEST("Are not equal test: one simple one createexample tree"){
    EncodingTreeNode *leafOne = new EncodingTreeNode('A');
    EncodingTreeNode *leafTwo = new EncodingTreeNode('B');
    EncodingTreeNode *simpleTree = new EncodingTreeNode(leafOne, leafTwo);

    EncodingTreeNode *exampleTree = createExampleTree();

    EXPECT(!areEqual(simpleTree, exampleTree));

    EncodingTreeNode *exampleTree2 = createExampleTree();

    EXPECT(areEqual(exampleTree, exampleTree2));


    deallocateTree(simpleTree);
    deallocateTree(exampleTree);
    deallocateTree(exampleTree2);

}

STUDENT_TEST("Are not equal test: example tree and one of its subtrees"){

    EncodingTreeNode *exampleTree = createExampleTree();

    EncodingTreeNode *subtree = exampleTree->one;

    EXPECT(!areEqual(exampleTree, subtree));

    deallocateTree(exampleTree);

}


STUDENT_TEST("decodeText, other inputs, example encoding tree") {
    EncodingTreeNode* tree = createExampleTree();
    EXPECT(tree != nullptr);

    Queue<Bit> messageBits = {}; // E
    EXPECT_EQUAL(decodeText(tree, messageBits), "");

    messageBits = {0, 1}; // T with incomplete message
    EXPECT_EQUAL(decodeText(tree, messageBits), "T");

    messageBits = {1, 0}; // another incomplete input
    EXPECT_EQUAL(decodeText(tree, messageBits), "");

    deallocateTree(tree);
}

STUDENT_TEST("unflattenTree, example encoding tree") {
    EncodingTreeNode* reference = createExampleTree(); // see diagram above
    Queue<Bit>  treeShape  = {1,0,1,1,0,0,0};
    Queue<char> treeLeaves = {'T','R','S','E'};
    EncodingTreeNode* tree = unflattenTree(treeShape, treeLeaves);

    EXPECT(areEqual(tree, reference));

    deallocateTree(tree);
    deallocateTree(reference);
}


STUDENT_TEST("decompress, small fixed input, simple encdoing tree") {
    EncodedData data = {
        {1,1,0,0,1,0,0},            // treeShape
        {'A','B','C','D'},          // treeLeaves
        {1, 1, 1, 0, 0, 1, 0, 0}   // messageBits
    };

    EXPECT_EQUAL(decompress(data), "DCBA");
}



STUDENT_TEST("encodeText, other inputs, example encoding tree") {
    EncodingTreeNode* reference = createExampleTree(); // see diagram above

    Queue<Bit> messageBits = {}; // nothing
    EXPECT_EQUAL(encodeText(reference, ""), messageBits);

    deallocateTree(reference);
}


STUDENT_TEST("flattenTree, empty tree") {
    EncodingTreeNode* reference = nullptr; // see diagram above
    Queue<Bit>  expectedShape  = {};
    Queue<char> expectedLeaves = {};

    Queue<Bit>  treeShape;
    Queue<char> treeLeaves;
    flattenTree(reference, treeShape, treeLeaves);

    EXPECT_EQUAL(treeShape,  expectedShape);
    EXPECT_EQUAL(treeLeaves, expectedLeaves);

    deallocateTree(reference);
}


STUDENT_TEST("buildHuffmanTree, input smaller than two chars") {
    EXPECT_ERROR(buildHuffmanTree(""));
}


STUDENT_TEST("Test end-to-end compress -> decompress, small fixed inputs") {
    Vector<string> inputs = {
        "COOL GUY",
        "wow I love to skate",
        "Heavy is the head that wears the crown",
    };

    for (string input: inputs) {
        EncodedData data = compress(input);
        string output = decompress(data);
        EXPECT_EQUAL(input, output);
    }
}


/* * * * * Provided Tests Below This Point * * * * */

PROVIDED_TEST("decodeText, small fixed inputs, example encoding tree") {
    EncodingTreeNode* tree = createExampleTree(); // see diagram above
    EXPECT(tree != nullptr);

    Queue<Bit> messageBits = {1,1}; // E
    EXPECT_EQUAL(decodeText(tree, messageBits), "E");

    messageBits = {1,0,1,1,1,0}; // SET
    EXPECT_EQUAL(decodeText(tree, messageBits), "SET");

    messageBits = {1,0,1,0,1,0,0,1,1,1,1,0,1,0,1}; // STREETS
    EXPECT_EQUAL(decodeText(tree, messageBits), "STREETS");

    deallocateTree(tree);
}

PROVIDED_TEST("unflattenTree, example encoding tree") {
    EncodingTreeNode* reference = createExampleTree(); // see diagram above
    Queue<Bit>  treeShape  = {1,0,1,1,0,0,0};
    Queue<char> treeLeaves = {'T','R','S','E'};
    EncodingTreeNode* tree = unflattenTree(treeShape, treeLeaves);

    EXPECT(areEqual(tree, reference));

    deallocateTree(tree);
    deallocateTree(reference);
}

PROVIDED_TEST("decompress, small fixed input, example encoding tree") {
    EncodedData data = {
        {1,0,1,1,0,0,0},            // treeShape
        {'T','R','S','E'},          // treeLeaves
        {0,1,0,0,1,1,1,0,1,1,0,1}   // messageBits
    };

    EXPECT_EQUAL(decompress(data), "TRESS");
}

PROVIDED_TEST("buildHuffmanTree, small fixed input, example encoding tree") {
    EncodingTreeNode* reference = createExampleTree(); // see diagram above
    EncodingTreeNode* tree = buildHuffmanTree("STREETTEST");
    EXPECT(areEqual(tree, reference));

    deallocateTree(reference);
    deallocateTree(tree);
}

PROVIDED_TEST("encodeText, small fixed inputs, example encoding tree") {
    EncodingTreeNode* reference = createExampleTree(); // see diagram above

    Queue<Bit> messageBits = {1,1}; // E
    EXPECT_EQUAL(encodeText(reference, "E"), messageBits);

    messageBits = {1,0,1,1,1,0};    // SET
    EXPECT_EQUAL(encodeText(reference, "SET"), messageBits);

    messageBits = {1,0,1,0,1,0,0,1,1,1,1,0,1,0,1}; // STREETS
    EXPECT_EQUAL(encodeText(reference, "STREETS"), messageBits);

    deallocateTree(reference);
}

PROVIDED_TEST("flattenTree, example encoding tree") {
    EncodingTreeNode* reference = createExampleTree(); // see diagram above
    Queue<Bit>  expectedShape  = {1,0,1,1,0,0,0};
    Queue<char> expectedLeaves = {'T','R','S','E'};

    Queue<Bit>  treeShape;
    Queue<char> treeLeaves;
    flattenTree(reference, treeShape, treeLeaves);

    EXPECT_EQUAL(treeShape,  expectedShape);
    EXPECT_EQUAL(treeLeaves, expectedLeaves);

    deallocateTree(reference);
}

PROVIDED_TEST("compress, small fixed input, example encoding tree") {
    EncodedData data = compress("STREETTEST");
    Queue<Bit>  treeShape   = {1,0,1,1,0,0,0};
    Queue<char> treeChars   = {'T','R','S','E'};
    Queue<Bit>  messageBits = {1,0,1,0,1,0,0,1,1,1,1,0,0,1,1,1,0,1,0};

    EXPECT_EQUAL(data.treeShape, treeShape);
    EXPECT_EQUAL(data.treeLeaves, treeChars);
    EXPECT_EQUAL(data.messageBits, messageBits);
}

PROVIDED_TEST("Test end-to-end compress -> decompress, small fixed inputs") {
    Vector<string> inputs = {
        "HAPPY HIP HOP",
        "Nana Nana Nana Nana Nana Nana Nana Nana Batman",
        "Research is formalized curiosity. It is poking and prying with a purpose. – Zora Neale Hurston",
    };

    for (string input: inputs) {
        EncodedData data = compress(input);
        string output = decompress(data);

        EXPECT_EQUAL(input, output);
    }
}

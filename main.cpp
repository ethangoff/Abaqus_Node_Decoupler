//Abaqus Node Decoupler - main routine
//Ethan Goff, April 2014

#include "mesh_input.h"
#include "compilation_options.h"

using namespace std;

#define VERBOSE

//We want to read in the data from a file, and find any shared edges, creating a new edge
//	node to replace the match's edge, decoupling the two elements.
//
//	The idea behind this algorithm is to use a hashmap to achieve linear search time
//	for large lists of elements. To do so, we'll create the map such that each edge node
//	is a key, with a matched value of true. We'll also create a vector of the new nodes we'll need
//	to add to the node list.
//
//	As we scan the file, we'll check if the nodes we read already exist in the map. If
//	they do, we'll add a brand new node to the vector, and we'll replace the duplicate
//	we found in the original element list. With this new node. This way, we'll do all the
//	work in a single pass, replacing duplicates as we find them.

int main()
{
    mesh_input testInput;
    testInput.init();
	testInput.ParseElements();
	testInput.CloseFiles();

    return 0;
}

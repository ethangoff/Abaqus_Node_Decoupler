//Abaqus Node Decoupler - mesh input class declarations
//Ethan Goff, April 2014

#include <fstream>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <map>
#include <stdint.h>
using namespace std;



#ifndef MESH_INPUT_H
#define MESH_INPUT_H


class mesh_input
{
	public:
		mesh_input();
		virtual ~mesh_input();
		void init();
		void ParseElements();
		void CloseFiles();

	protected:
	private:
    	void CopyHeader();
		void CopyInitialNodes();
		void CopyElementsFromTemporary(uint16_t elementsToRead);
		void CopyPostlude();
		uint16_t DuplicateNode(uint16_t nodeNumber);
		void ProcessElement(char* inputElementString, int numberOfNodes, char* outputElementString);
        void WriteCohesiveElements();



		fstream inputFile;
		uint16_t numberOfNodes;
		uint16_t elementsRead;
        uint16_t firstCohesiveElement;
        uint16_t lastCohesiveElement;
	    streampos positionOfNodesInInputFile;
		streampos positionOfFirstElementInInputFile;

		ofstream outputFile;
		streampos positionToAppendNodesInOutputFile;

		fstream elementsOutput;

		//Our nodes map will have integer keys which correspond with
		//	boolean values. As we add nodes, we'll set the boolean
		//	to true, which will allow us to quickly later see if a node
		//  has been encountered in the past.
		static unordered_map <uint16_t, bool> EncounteredNodesTable;

		//Our nodes map will have integer keys which correspond with
		//	boolean values. As we add nodes, we'll set the boolean
		//	to true, which will allow us to quickly later see if a node
		//  has been encountered in the past.
		static map <pair<uint16_t, uint16_t>, bool> EncounteredElementsTable;

		static vector < vector<uint16_t> > cohesiveElements;


};

#endif // MESH_INPUT_H

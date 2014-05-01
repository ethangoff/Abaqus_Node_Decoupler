//Abaqus Node Decoupler - mesh input class definitions
//Ethan Goff (with support from Emma Roudabush), April 2014

#include "mesh_input.h"

//Standard C libraries used
#include <stdlib.h>
#include <string.h>

#define OUTPUT_FILE_POSTFIX "_out"
#define INPUT_FILE_NAME "notchedcrack"
#define INPUT_FILE_EXTENSION ".inp"

#define ELEMENT_TYPE_OFFSET 16
#define LINE_BUFFER_SIZE 200
#define BASE 10

unordered_map <uint16_t, uint16_t> mesh_input::EncounteredNodesTable;
map <pair<uint16_t, uint16_t>, bool> mesh_input::EncounteredElementsTable;
vector < vector<uint16_t> > mesh_input::cohesiveElements;

char* itoa(int num, char* str, int base);


mesh_input::mesh_input()
{
        elementsRead = 0;
		numberOfNodes = 0;
	    positionOfNodesInInputFile = 0;
		positionOfFirstElementInInputFile = 0;
		positionToAppendNodesInOutputFile =0;
}
a
mesh_input::~mesh_input()
{
	//dtor
}

//Closes both the input and output files
void mesh_input::CloseFiles()
{
	#ifdef VERBOSE
	//Inspect Dupes
	for( vector< vector<uint16_t> >::iterator i = cohesiveElements.begin(); i != cohesiveElements.end(); ++i)
	{
		cout << "Cohesive Element contains the following nodes: ";
		for(vector<uint16_t>::iterator j = (*i).begin(); j != (*i).end(); ++j)
		{
			cout << *j << ", ";
		}
		cout << endl;
	}
	#endif // VERBOSE


    //Close the files
	inputFile.close();
	outputFile.close();
	outputFile.close();
}


//Preps the file resources for use and copies the header
//	section of the input file, as well as the initial nodes
void mesh_input::init()
{
#ifdef USERINPUT
    //cout << "Input File name: ";
    //cin >> inputFileName;
#endif // USERINPUT

    //Open the input and output files
    char inputFileName[LINE_BUFFER_SIZE] = INPUT_FILE_NAME;
    char outputFileName[LINE_BUFFER_SIZE] = "";
	strcpy(outputFileName, inputFileName);
    strcat(outputFileName, OUTPUT_FILE_POSTFIX);
    strcat(inputFileName, INPUT_FILE_EXTENSION);
    strcat(outputFileName, INPUT_FILE_EXTENSION);

	//We'll have three files - an input to be read, and output to write to, and a temporary buffer file
	//	to which we'll pipe the elements as we process them for duplicate nodes
    inputFile.open(inputFileName, fstream::in);
    outputFile.open(outputFileName, ofstream::out | ofstream::trunc);
    elementsOutput.open("temp_output_buffer",  fstream::in | fstream::out | ofstream::trunc);

	//Pipe the static portion of the input file (the beginning through the end of the nodes list) to the
	//	front of the output file
	CopyHeader();
	CopyInitialNodes();
}

//Copies the header section of the input file to the output file
void mesh_input::CopyHeader()
{
    //Traverse to the beginning of the nodes list
    char currentLine[LINE_BUFFER_SIZE] = "";
    while(strncmp(currentLine, "*Node", 5)  != 0)
    {
       inputFile.getline(currentLine, LINE_BUFFER_SIZE);
       outputFile << currentLine <<endl;
	}

    positionOfNodesInInputFile = inputFile.tellg();
}

//Copies the initial (unchanged) nodes list of the input file to the output file,
//	and records the position at which this addition ends in the output file
//	so we can write from this point later
void mesh_input::CopyInitialNodes()
{
    //Traverse to the beginning of the elements list
	char currentLine[LINE_BUFFER_SIZE] = "";
    while(true)
    {
		inputFile.getline(currentLine, LINE_BUFFER_SIZE);
		if(strncmp(currentLine, "*Elem", 5) != 0)
		{
		    positionOfFirstElementInInputFile = inputFile.tellg();
			outputFile << currentLine <<endl;
			numberOfNodes++;
		}
		else
			break;
    }
   	char nextChar = ' ';
    outputFile.put(nextChar);

    positionToAppendNodesInOutputFile= outputFile.tellp();
}


//Copies the rest of the input file, following the element lists, to the
//	output file
void mesh_input::CopyPostlude()
{
	char currentLine[LINE_BUFFER_SIZE] = "";
	int test = 0;
	while( strncmp(currentLine, "*End Step", 7) != 0 )
	{
		test++;
		if(test >84)
			test=0;
		inputFile.getline(currentLine, LINE_BUFFER_SIZE);
		outputFile << currentLine << endl;
	}
}

//Duplicates a given node from the input file and appends the
//	duplicate to the tail end of the nodes list in the output file
uint16_t mesh_input::DuplicateNode(uint16_t nodeNumber)
{
	char unusedBuffer[LINE_BUFFER_SIZE] = "";

	streampos returnPosition = inputFile.tellg();

	inputFile.seekg(positionOfNodesInInputFile);
	//Find the node to duplicate
    for(int i = 0; i < nodeNumber-1; i++)
    {
        inputFile.getline(unusedBuffer, LINE_BUFFER_SIZE);
    }
    //Seek to the appension point for the output file
	outputFile.seekp(positionToAppendNodesInOutputFile);


	char nextChar = ' ';
	//Technically Optional Formatting
	inputFile.get(nextChar);
	inputFile.get(nextChar);
	while(nextChar == ' ')
	{
		outputFile.put(nextChar);
		inputFile.get(nextChar);
	}

	//First, we want to append the *new* node number
	//Create a character array to hold the new node number as a string
	char newNodeNumber[8];
	itoa(++numberOfNodes, newNodeNumber, 10);
	//Add a comma at the end
	char comma[2] = {',','\0'};
	strcat(newNodeNumber, comma);
	//Append the node number as a character string
	outputFile.write(newNodeNumber, strlen(newNodeNumber) );

	//Then, we ignore the old node number in the inputFile, seeking to the beginning of the coordinates list
	inputFile.ignore(LINE_BUFFER_SIZE, ',');

	//Finally, we copy the node's list coordinates character-for-character
	while(nextChar != '\n')
	{
		inputFile.get(nextChar);
		outputFile.put(nextChar);
	}

	//Update our record of where to append new nodes
    positionToAppendNodesInOutputFile = outputFile.tellp();

    inputFile.seekg(returnPosition);
    return numberOfNodes;
}

//Pipes the elements that were placed in the temporary buffer
//	into the actual output file
void mesh_input::CopyElementsFromTemporary(uint16_t elementsToRead)
{
	elementsOutput.seekg(0);
	char currentLine[LINE_BUFFER_SIZE] = "";
	for(int i=0;i<elementsToRead;i++)
	{
		elementsOutput.getline(currentLine, LINE_BUFFER_SIZE);
		outputFile << currentLine << endl;
	}
}


//The main loop that steps through the element lists and pipes the
//	possibly modified elements to the temporary output buffer
void mesh_input::ParseElements()
{
	//Seek to the elements list in the input file
	inputFile.seekg(positionOfFirstElementInInputFile);

	//Get the first line of the elements list (should be a comment with node type
	//	information)
    char currentLine[LINE_BUFFER_SIZE] = "";
	inputFile.getline(currentLine, LINE_BUFFER_SIZE);
    uint8_t nodesPerElement = 0;

	//Read through the elements list, processing and outputing each
	while( (inputFile.rdstate() & std::ifstream::eofbit ) != 1 )
    {
    	//If the line is a comment line, we need to check if its a new element type
    	//	or if it's the ELSET directive (which directly follows the entire elements list)
    	if(currentLine[0] == '*')
		{
				if(currentLine[3] == 's') // "*ELS..."
				{
                    //Before copying the remainder of the mesh input file,
                    //  write the cohesive elements out to the temporary
                    WriteCohesiveElements();
                    //Copy the elset and the subsequent line
                    elementsOutput << "*Elset, elset=BULK, generate\n";
                    elementsOutput << "1, " << firstCohesiveElement-1 << ", 1\n";
                    //Copy the elset and the subsequent line
                    elementsOutput << "*Elset, elset=COHES, generate\n";
                    elementsOutput << firstCohesiveElement << ", " << lastCohesiveElement << ", 1\n";

					elementsRead++;
					CopyElementsFromTemporary(elementsRead+cohesiveElements.size()+3);


					CopyPostlude();
					return;
				}
				else if(currentLine[3] == 'e') // "*Ele..."
				{
					elementsOutput << currentLine << endl;
					elementsRead++;
					//Because the element types aren't strictly character-aligned
					//	we'll search through the rest of the comment line to find
					//	a digit representing the node count
					uint8_t searchTo = strlen(currentLine);
					for(int i=ELEMENT_TYPE_OFFSET; i <= searchTo; i++)
					{
							if( isdigit(currentLine[i]) )
							{
								nodesPerElement = atoi(currentLine+i);
								currentLine[0] = '\0';
								break;
							}
					}
				}
				else
				{
					//If we manage to skip the ELSET directive... or run into some other comment
					cout << "ERR";
				}
		}
		else
		{
			char outputLine[LINE_BUFFER_SIZE] = "";
			ProcessElement(currentLine, nodesPerElement, outputLine);
			elementsOutput << outputLine << endl;
			elementsRead++;
		}
		inputFile.getline(currentLine, LINE_BUFFER_SIZE);
	}


}

//Process a single element - read through its nodes, checking if any have been
//	encountered before, duplicating them and replacing the reference to them if
//	this is the case.
void mesh_input::ProcessElement(char* inputElementString, int numberOfNodes, char* outputElementString)
{
	char* token;
	char output[10];
	const char delim[2] = ",";
	uint16_t number, newNode;
	uint16_t originalNodesList[3];
	uint16_t newNodesList[3];

	token = strtok(inputElementString, delim);

#ifdef VERBOSE
	cout << "Working on Element: " << inputElementString << endl;
#endif // VERBOSE

	for(int i = 0; token != NULL ; i++)
	{
		if (i == 0) // it is the first element and therefore the first number
		{
			strcat(outputElementString, token);
		}
		else // not first element
		{
			//Evaluate the next node within the current element.
			number = (uint16_t)atoi(token);
			originalNodesList[i-1] = number;


			//If the node	has been encountered, currentNodeIsRedundant will point
            //  to a TRUE value. If not, it will point to a FALSE value, which is the
            //	default value for a bool type (our table's value type).

            //In the case that  a node hasn't been encountered, add it to the table
            //	with a value of itself. Add the number back unchanged to the output
            //	element string.
			if( !EncounteredNodesTable[number])
			{
				EncounteredNodesTable[number] = number;
				strcat(outputElementString, ", ");
				strcat(outputElementString, token);
				newNodesList[i-1] = number;
			}
            //In the case that  a node has already been encountered, duplicate it.
            //  Replace the node in the map to be the duplicate. Add the number of
            // the new, duplicated node to the output element string.
			else
			{
				newNode = DuplicateNode(number);
				EncounteredNodesTable[number] = newNode;
				itoa(newNode, output, BASE);
				strcat(outputElementString, ", ");
				strcat(outputElementString, output);
				newNodesList[i-1] = newNode;

			}
		}
		token = strtok(NULL, delim);
	}

#ifdef VERBOSE
	cout << "original nodes list: ";
	for(int i=0; i<3;i++)
	{
		cout << originalNodesList[i] << ' ';
	}
	cout << endl;
	cout << "new nodes list:      ";
	for(int i=0; i<3;i++)
	{
		cout << newNodesList[i] << ' ';
	}
	cout << endl;
	cout << endl;

#endif // VERBOSE


	//Find the edges of the element - at the moment, this assumes linearity and 2 dimensions
	for(int i = 0; i<numberOfNodes-1; i++)
	{
		for(int j =i+1; j<numberOfNodes; j++)
		{
			pair<uint16_t, uint16_t> newPair = make_pair( originalNodesList[i], originalNodesList[j] ) ;
			if( !EncounteredElementsTable[newPair])
				EncounteredElementsTable[newPair] = true;
			else
			{
				//Create a cohesive element for later
				//Our cohesive element will contain originalNodesList[i], originalNodesList[j], newNodesList[i], and newNodesList[j]
				vector<uint16_t> newCohesiveElement;
				newCohesiveElement.push_back( (originalNodesList[i]) );
				newCohesiveElement.push_back( originalNodesList[j] );
				newCohesiveElement.push_back( newNodesList[i] );
				newCohesiveElement.push_back( newNodesList[j] );
				cohesiveElements.push_back(newCohesiveElement);
			}
		}
	}

}

void mesh_input::WriteCohesiveElements()
{
    firstCohesiveElement = elementsRead;
    elementsOutput << "*Element, type=COH2D4\n";
    int numberOfCohesiveElements = cohesiveElements.size();
    lastCohesiveElement = elementsRead + numberOfCohesiveElements - 1;
    for(int i=0; i<numberOfCohesiveElements; i++)
    {
        elementsOutput << elementsRead+i << ", ";
        for(int j=0; j<3; j++)
        {
            elementsOutput << (cohesiveElements[i])[j] << ", ";
        }
        elementsOutput << (cohesiveElements[i])[3];
        elementsOutput.put('\n');
    }


}

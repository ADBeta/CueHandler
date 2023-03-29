/*******************************************************************************
* This file is part of psx-co

* psXtract is a program to extract filesystem files/folders and CDDA audio from
* PSX/PS1 games.
*
* This file handles input .cue files and extracts all the important information
* from them. This is borrowed and improved from psx-comBINe
*
* (c) ADBeta
*******************************************************************************/
#include "CueHandler.hpp"
#include "TeFiEd.hpp"

#include <iostream>
#include <vector>

//TODO 
//175		substrNonEmpty
//484		cleanFILE


/*** Error message handler ****************************************************/
namespace errStr {
const char* warnMsg = "Warning: CueHandler: ";
const char* errMsg = "Error: CueHandler: ";

const char* invalidCueFile = "The input file is not a .cue file\n";
const char* invalidTRACK = "A TRACK in the .cue file is invalid or corrupt\n";
const char* invalidFILE = "A FILE in the .cue file is invalid or corrupt\n";

const char* noFilename = "A FILE In the .cue has no filename.\n";
const char* unknownFILE = "A FILE being validated is of type UNKNOWN\n";

const char* overTRACKMax = "More than 99 TRACKs exist. Not a standard CD\n";
const char* unknownTRACK = "A TRACK being validated is of type UNKNOWN\n";

const char* overINDEXMax = "More than 99 INDEXs exist. Not a standard CD\n";
const char* sectorBytesWrong = "An INDEXs' BYTES do not allign with the\
 Sector size. Incorrect TRACK MODE or corrupt image.\n";

const char* sectByte = "Cannot convert timestamp - Bytes in dump do not match\
 the Sector Size. This is a corrupted or modified dump.\n";

const char* timestampLength = "The timestamp string is not the right size\n";
const char* timeOverMax = "INDEX Timestamp exceeds 99 Minutes.\n";

const char* createFail = "Failed to create a .cue file to output data to\n";
const char* fileEmpty = "A non-existent FILE was attempted to be read.\n";

const char* invalidCmd = ".cue file contains an unrecognised command (line)\n";

const char* badPushTrack = "Attempted to push a TRACK, but no FILE exists\n";
const char* badPushIndex = "Attempted to push an INDEX, but no TRACK exists\n";

} //namespace errStr

/*** Internal Error Handling **************************************************/
void CueHandler::handleCueError(const char* msg) {
	//if the strictness is at 0, just continue, don't worry about the error
	if(this->strictLevel == 0) return;
	
	//Strictness 1 is a warning
	if(this->strictLevel == 1) {
		std::cerr << errStr::warnMsg << msg;
		return;
	}
	
	//Strictness 2 is an error and exit
	if(this->strictLevel == 2) {
		std::cerr << errStr::errMsg << msg;
		exit(EXIT_FAILURE);
	}
}

void CueHandler::forceCueError(const char* msg) {
	std::cerr << errStr::errMsg << msg;
	exit(EXIT_FAILURE);
}



/*** CueHandler Functions *****************************************************/
CueHandler::CueHandler(const std::string filename) {
	//Set the TeFiEd file object to the passed filename string
	cueFile = new TeFiEd(filename);
	
	//Set safety size limit of 100KB
	cueFile->setByteLimit(102400);
	
	//Make sure the input filename is a valid .cue file. Exit if not
	//Validate will end execution or warn if there are issues
	validateCueFilename(filename);
		
	//Debug option
	//cueFile->setVerbose(true);
}

CueHandler::~CueHandler() {
	//Delete the TeFiEd object
	delete cueFile;
	
	//Clear the FileData struct RAM
	FILE.clear();
	FILE.shrink_to_fit();
}










/*** Enum mapped strings for type detection ***********************************/
const std::string t_FILE_str[] = {
	"UNKNOWN", "BINARY", "MP3"
};   

const std::string t_TRACK_str[] = {
	"UNKNOWN", "AUDIO", "CDG", "MODE1/2048", "MODE1/2352", "MODE2/2336", 
	"MODE2/2352", "CDI/2336", "CDI/2352"
};

/*** FILE Vector Functions ****************************************************/
t_LINE CueHandler::LINEStrToType(const std::string lineStr) {	
	//If line is empty return EMPTY
	if(lineStr.length() == 0) return t_LINE::EMPTY;

	//Check against known strings for types
	if(lineStr.find("    INDEX") != std::string::npos) return t_LINE::INDEX;
	if(lineStr.find("  TRACK") != std::string::npos) return t_LINE::TRACK;
	if(lineStr.find("FILE \"") != std::string::npos) return t_LINE::FILE;
	
	//Remark type
	if(lineStr.find("REM ") != std::string::npos) return t_LINE::REM; 
	
	//Failure to find any known string means it's an invalid line.
	//Error or warn depending on user settings
	handleCueError(errStr::invalidCmd);
	//Return invalid line type, incase warn or ignore
	return t_LINE::INVALID; 
}

t_TRACK CueHandler::TRACKStrToType(const std::string trackStr) {
	//The TRACK Type substring is the 3rd word
	std::string typeStr = getWord(trackStr, 3);
	
	//If the TRACK string is empty, this is extremely corrupt. force and error
	if(typeStr == "") forceCueError(errStr::invalidTRACK);
	
	//Go through all elements in t_TRACK (MAX_TYPES)
	for(int compType = 0; compType < (int)t_TRACK::MAX_TYPES; compType++) {
		//If the input string and the //TODO TRACKType string match
		if(typeStr.compare(t_TRACK_str[compType]) == 0) {
			//Return the matched type as enum int
			return (t_TRACK)compType;
		}
	}
	
	//If nothing matches, let the user config decide to error, warn or ignore
	handleCueError(errStr::invalidTRACK);
	//Return UNKNOWN if the system warns or ignores the error.
	return t_TRACK::UNKNOWN;
}

t_FILE CueHandler::FILEStrToType(const std::string fileStr) {
	//The FILE type is after the last '"'. Remove any leading or trailing ' '
	size_t lastQuote = fileStr.find_last_of("\"");
	//TODO
	std::string typeStr = substrNonEmpty(fileStr, lastQuote, std::string::npos);
	
	//If the FILE type string is empty, this is extremely corrupt. Force error
	if(typeStr == "") forceCueError(errStr::invalidFILE);
	
	//Go through all elements in t_FILE_str and string compare them to input
	for(int compType = 0; compType < (int)t_FILE::MAX_TYPES; compType++) {
		//If the input string and the TRACKType string match
		if(typeStr.compare(t_FILE_str[compType]) == 0) {
			//Return the matched type as enum int
			return (t_FILE)compType;
		}
	}
	
	//If nothing matched, let the user config decide to error, warn or ignore
	handleCueError(errStr::invalidFILE);
	//Return UNKNOWN if nothing matched and the system doesn't error
	return t_FILE::UNKNOWN;
}

/*** Type to String conversion ************************************************/
std::string CueHandler::FILETypeToStr(const t_FILE fileType) {
	std::string typeOut;
	
	//Go through all elements in t_FILE and match it with input type
	for(int compType = 0; compType < (int)t_FILE::MAX_TYPES; compType++) {
		//If the input type matches type in enum 
		if(fileType == (t_FILE)compType) {
			//Set the output string
			typeOut = t_FILE_str[compType];
		}
	}
	
	//Return the matched type string
	return typeOut;
}

std::string CueHandler::TRACKTypeToStr(const t_TRACK trackType) {
	std::string typeOut;	
	
	//Go through all elements in t_TRACK_str (current type string)
	for(int compType = 0; compType < (int)t_TRACK::MAX_TYPES; compType++) {
		//If the input type matches type in enum 
		if(trackType == (t_TRACK)compType) {
			//Set the output string
			typeOut = t_FILE_str[compType];
		}
	}
	
	//Return the matched type string
	return typeOut;
}


/*** Data Validation functions. Returns specific error codes ******************/
void CueHandler::validateCueFilename(std::string cueStr) {
	size_t npos = std::string::npos;

	//Make sure the file extension is .cue or .CUE 
	if(cueStr.find(".cue") == npos && cueStr.find(".CUE") == npos) {
		forceCueError(errStr::invalidCueFile);
	}
}

void CueHandler::validateFILE(const FileData &refFILE) {
	//A file is invalid if:
	//No FILENAME
	if(refFILE.FILENAME == "") {
		//if there is no FILENAME this is a badly corrupted FILE
		forceCueError(errStr::noFilename);
	}
	
	//UNKNOWN file type 
	if(refFILE.TYPE == t_FILE::UNKNOWN) {
		//Depending on strictness level, warn error or ignore.
		handleCueError(errStr::unknownFILE);
	}
}

void CueHandler::validateTRACK(const TrackData &refTRACK) {
	//A TRACK is invalid if:
	//It is over the 99th TRACK in a file
	if(refTRACK.ID > 99) {
		//Let the user settings decide if error, warn or ignore
		handleCueError(errStr::overTRACKMax);
	}
	
	//it is an UNKNOWN type
	if(refTRACK.TYPE == t_TRACK::UNKNOWN) {
		handleCueError(errStr::unknownTRACK);
	}
}

void CueHandler::validateINDEX(const IndexData &refINDEX) {
	//An INDEX is invalid if:
	//There are more than 99 of them in a TRACK
	if(refINDEX.ID > 99) {
		handleCueError(errStr::overINDEXMax);
	}

	//Check if BYTES is divisible by SECTOR size TODO
	/*
	if(refINDEX.BYTES % 2352 != 0) {
		errorMsg(0, "validateINDEX",
		         "INDEX BYTES not divisible by SECTOR size");
		return 2;
	}
	*/
}

/*** CUE Metadata structure Adding ********************************************/
void CueHandler::pushFILE(const std::string FN, const t_FILE TYPE) {
	//Temporary FILE object
	FileData tempFILE;
	
	//Set the FILE Parameters
	tempFILE.FILENAME = FN;
	tempFILE.TYPE = TYPE;
	
	//Validate will end execution or warn if there are issues
	validateFILE(tempFILE);
	
	//Push tempFILE to the FILE vect
	FILE.push_back(tempFILE);
}

void CueHandler::pushTRACK(const unsigned int ID, const t_TRACK TYPE) {
	//Temporary TRACK object
	TrackData tempTRACK;
	//Set the TRACK Parameters
	tempTRACK.ID = ID;
	tempTRACK.TYPE = TYPE;
	
	//Validate will end execution or warn if there are issues
	validateTRACK(tempTRACK);
	
	//Get a pointer to the last entry in the FILE object
	FileData *pointerFILE = &FILE.back();
	//Push the tempTRACK to the back of the pointer 
	pointerFILE->TRACK.push_back(tempTRACK);
}

void CueHandler::pushINDEX(const unsigned int ID, const unsigned long BYTES) {
	//Temporary INDEX object
	IndexData tempINDEX;
	//Set INDEX parameters
	tempINDEX.ID = ID;
	tempINDEX.BYTES = BYTES;
	
	//Validate will end execution or warn if there are issues
	validateINDEX(tempINDEX);
	
	//Get a pointer to the last FILE and TRACK Object
	TrackData *pointerTRACK = &FILE.back().TRACK.back();
	//Push the INDEX to the end of current file
	pointerTRACK->INDEX.push_back(tempINDEX);
}










/*** CUE String Generation ****************************************************/
std::string CueHandler::generateFILELine(const FileData &refFILE) {
	//Validate will end execution or warn if there are issues
	validateFILE(refFILE);
	
	std::string outputLine = "FILE ";
	
	//Add " to the front and back of FILENAME, and a space after the second "
	//Append the FILENAME to the output
	outputLine.append("\"" + refFILE.FILENAME + "\" ");
	//Append the type
	outputLine.append(FILETypeToStr(refFILE.TYPE));
	
	return outputLine;
}
	
//Converts TrackData Object into a string which is a CUE file line
std::string CueHandler::generateTRACKLine(const TrackData &refTRACK) {
	//Validate will end execution or warn if there are issues
	validateTRACK(refTRACK);
	
	std::string outputLine = "  TRACK ";
	
	//Append the TRACK ID (padded to 2 length) and a space
	outputLine.append( padIntStr(refTRACK.ID, 2) + " ");
	//Append the TRACK TYPE
	outputLine.append(TRACKTypeToStr(refTRACK.TYPE));
	
	return outputLine;
}
	
//Converts IndexData Object into a string which is a CUE file line
std::string CueHandler::generateINDEXLine(const IndexData &refINDEX) {
	//Validate will end execution or warn if there are issues
	validateINDEX(refINDEX);
	
	std::string outputLine = "    INDEX ";
	
	//Append the INDEX ID (padded to 2 length) and a space
	outputLine.append( padIntStr(refINDEX.ID, 2) + " ");
	//Append the INDEX TIMESTAMP (string
	outputLine.append( bytesToTimestamp(refINDEX.BYTES) );
	
	return outputLine;
}







/*** CUE Data handling ********************************************************/
std::string CueHandler::getFilenameFromLine(const std::string line) {

	//Get the First and last quote in the string
	size_t fQuote = line.find('\"') + 1;
	size_t lQuote = line.find('\"', fQuote);
	
	//If the last quote is npos (could detect on first too, but may be slower)
	if(lQuote == std::string::npos) forceCueError(errStr::noFilename);
	
	//Return a substring of the input fron fQuote, of size first - last 
	return line.substr(fQuote, lQuote - fQuote);
}


void CueHandler::getCueData() {
	//Clean the FILE vector RAM
	FILE.clear();
	FILE.shrink_to_fit();

	//Read in the .cue file, with error handling
	if(cueFile->read() != 0) exit(EXIT_FAILURE); 
	
	//Make sure the Line Ending type is Unix, not DOS
	cueFile->convertLineEnding(LineEnding::Unix);

	//Go through all the lines in the cue file.
	for(size_t lineNum = 1; lineNum <= cueFile->lines(); lineNum++) {
		//Copy the current line to a new string
		std::string cLineStr = cueFile->getLine(lineNum);
		
		//Get the type of the current line
		t_LINE cLineType = LINEStrToType(cLineStr);
		
		//If the current line is invalid, exit with error message
		if(cLineType == t_LINE::INVALID) forceCueError(errStr::invalidCmd);
		
		//If the current line is a REM command
		if(cLineType == t_LINE::REM) {
			//TODO Decide what to do with REMARKS
			//std::cout << "REMARK at line: " << lineNo << "\t message: " <<
			//cLineStr << std::endl;
		}
		
		//If the current line is a FILE command
		if(cLineType == t_LINE::FILE) {
			//Get the FILE type string, and the FILENAME String
			t_FILE fileType = FILEStrToType(cLineStr);
			std::string fileName = getFilenameFromLine(cLineStr);
			
			//push new FILE to the stack
			pushFILE(fileName, fileType);
		}
		
		//If the current line is a TRACK command
		if(cLineType == t_LINE::TRACK) {
			//Make sure a FILE is availible to push to
			if(FILE.empty() == true) forceCueError(errStr::badPushTrack);
		
			//Get ID (second word), and TYPE
			unsigned int lineID = std::stoi(getWord(cLineStr, 2));
			t_TRACK lineTYPE = TRACKStrToType(cLineStr);
			
			//Push new TRACK to the FILE vector
			pushTRACK(lineID, lineTYPE);
		}
		
		//INDEX line type	
		if(cLineType == t_LINE::INDEX) {
			//Make sure a TRACK is availible to push to
			if(FILE.back().TRACK.empty() == true) {
				forceCueError(errStr::badPushIndex);
			}
			
			//Get ID (second word), and timestamp (third word)
			unsigned int lineID = std::stoi(getWord(cLineStr, 2));
			unsigned long lineBytes = timestampToBytes(getWord(cLineStr, 3));
			
			//Push new INDEX to TRACK sub-vector
			pushINDEX(lineID, lineBytes);
		}
	}
}


//TODO
int CueHandler::combineCueFiles(CueHandler &combined, const std::string outBin,
                                const std::vector <unsigned long> offsetBytes) {
	//Clear the pointer object FILE vector RAM
	//TODO
	combined.cleanFILE();
		
	//Push the passed filename (relative, not output) to the FILE Vector
	combined.pushFILE(outBin, this->FILE[0].TYPE);
	
	//Go through all the callers' FILE vector
	for(size_t cFile = 0; cFile < this->FILE.size(); cFile++) {
		//Temporary FILE Object
		FileData pFILE = this->FILE[ cFile ];
		
		//Go through all the TRACKs
		for(size_t cTrack = 0; cTrack < pFILE.TRACK.size(); cTrack++) {
			//Temporary TRACK Object
			TrackData pTRACK = pFILE.TRACK[ cTrack ];
			
			//Push pTRACKs info to the output file vect
			combined.pushTRACK(pTRACK.ID, pTRACK.TYPE);
			
			//Go through all INDEXs
			for(size_t cIndex = 0; cIndex < pTRACK.INDEX.size(); cIndex++) {
				//Temporary INDEX object
				IndexData pINDEX = pTRACK.INDEX[ cIndex ];
				
				//Push pINDEX info to the combined ref object
				unsigned long cIndexBytes = pINDEX.BYTES + offsetBytes[cFile];
				combined.pushINDEX(pINDEX.ID, cIndexBytes);
			}
		}
	}
	
	return 0;
}

void CueHandler::outputCueFile() {
	//Try to create a new TeFiEd file. Exit if not
	if(cueFile->create() != 0) forceCueError(errStr::createFail);
	
	//Go through all the callers' FILE vector
	for(size_t cFile = 0; cFile < this->FILE.size(); cFile++) {
		//Temporary FILE Object
		FileData pFILE = this->FILE[ cFile ];
		
		//Print Current FILE string to the cue file
		cueFile->append( generateFILELine(pFILE) );
		
		//Go through all the TRACKs
		for(size_t cTrack = 0; cTrack < pFILE.TRACK.size(); cTrack++) {
			//Temporary TRACK Object
			TrackData pTRACK = pFILE.TRACK[ cTrack ];
			
			//Print current TRACK string to the cue file
			cueFile->append( generateTRACKLine(pTRACK) );
			
			//Go through all INDEXs
			for(size_t cIndex = 0; cIndex < pTRACK.INDEX.size(); cIndex++) {
				//Temporary INDEX object
				IndexData pINDEX = pTRACK.INDEX[ cIndex ];
				
				//Print current INDEX string to the cue file
				cueFile->append( generateINDEXLine(pINDEX) );
			}
		}
	}
	
	//Write the cue data to the file
	cueFile->overwrite();
}

void CueHandler::printFILE(FileData & pFILE) {
	//Check if pFILE is empty, error if attempted read from empty
	if(pFILE.FILENAME.empty()) forceCueError(errStr::fileEmpty);

	//Print filename and data
	std::cout << "FILENAME: " << pFILE.FILENAME;
	std::cout << "\t\tTYPE: " << t_FILE_str[(int)pFILE.TYPE] << "\n";
	//Seperator
	std::cout << "----------------------------------------------------------\n";

	//Print all TRACKs in vector held by FILE
	for(size_t tIdx = 0; tIdx < pFILE.TRACK.size(); tIdx++) {
		//Set TrackData object to point to current TRACK
		TrackData pTRACK = pFILE.TRACK[tIdx];
		
		//Print track number (TrackIndex + 1 to not have 00 track) and PREGAP:
		std::cout << "TRACK " << padIntStr(pTRACK.ID, 2);
		
		//Print TRACK TYPE variable
		std::cout << "        TYPE: " << t_TRACK_str[(int)pTRACK.TYPE] << "\n";
		
		//Print all INDEXs contained in that TRACK
		for(size_t iIdx  = 0; iIdx < pTRACK.INDEX.size(); iIdx++) {
			IndexData pINDEX = pFILE.TRACK[tIdx].INDEX[iIdx];
			
			//Print the index number
			std::cout << "  INDEX " << padIntStr(pINDEX.ID, 2)
			//Print the raw BYTES format 
			<< "    BYTES: " << padIntStr(pINDEX.BYTES, 9, ' ')
			//Print the timestamp version
			<< "    TIMESTAMP: " << bytesToTimestamp(pINDEX.BYTES) << "\n";
		}
		
		//Print a blank line to split the TRACK fields and flush the buffer
		std::cout << std::endl;
	}
}

/** Helper Functions **********************************************************/
/*******************************************************************************
The timestamp is in Minute:Second:Frame format.
There are 75 sectors per second, 2352 bytes per sector. If any number of bytes 
is not divisible by the sector size, it is a malformed or corrupted dump, so
the program will print an error message and exit.

Throughout this code I am trying to use divide numbers, then do modulo ops in 
that order so the compiler stands some chance of optimizing, useing the 
remainder of the ASM div operator.
*******************************************************************************/
std::string CueHandler::bytesToTimestamp(const unsigned long bytes) {
	//There are 2352 byte per sector. Calculate how many sectors are in the file
	unsigned long sectors = bytes / 2352;
	
	//Error check if the input is divisible by a sector. Exit if not
	if(bytes % 2352 != 0) forceCueError(errStr::sectByte);
	
	//75 sectors per second. Frames are the left over sectors from a second
	unsigned short seconds = sectors / 75;
	unsigned short rFrames = sectors % 75;
	
	//Convert seconds to minutes. Seconds is the remainder of itself after / 60
	unsigned short minutes = seconds / 60;
	seconds = seconds % 60;
	
	//If minutes exceeds 99, there is probably an error due to Audio CD Standard
	if(minutes > 99) forceCueError(errStr::timeOverMax);
	
	std::string timestamp;
	//Now the string can be formed from the values. Need to 0 pad each value
	timestamp.append(padIntStr(minutes, 2) + ":" + padIntStr(seconds, 2)
	                           + ":" + padIntStr(rFrames, 2));
	
	return timestamp;
}

unsigned long CueHandler::timestampToBytes(const std::string timestamp) {
	//Make sure the string input is long enough to have xx:xx:xx timestamp
	if(timestamp.length() != 8) forceCueError(errStr::timestampLength);

	//Strip values from the timestamp. "MM:SS:ff" ff = sectors
	unsigned short minutes = std::stoi(timestamp.substr(0, 2));
	unsigned short seconds = std::stoi(timestamp.substr(3, 2));
	unsigned short frames  = std::stoi(timestamp.substr(6, 2));
	
	//Add minutes to the seconds for sector calculation
	seconds += (minutes * 60);
	
	//75 sectors per second, plus frames left over in the timestamp	
	unsigned long sectors = (seconds * 75) + frames;
	
	//There are 2352 bytes per sector.
	unsigned long bytes = sectors * 2352;
	
	//Error check if the input is divisible by a sector. Exit if not
	if(bytes % 2352 != 0) forceCueError(errStr::sectByte);
	
	return bytes;
}

//Coppied from TeFiEd to avoid static class methods
std::string CueHandler::getWord(const std::string input, unsigned int index) {
	//If index is 0, set it to 1. always 1 indexed
	if(index == 0) index = 1;
	
	//Create output string object
	std::string output;
	
	//Start and end of word, and current word found.
	size_t wordStart = 0, wordEnd = 0, wordIndex = 0;
	
	//Find the start and end of a word
	do {
		//Get the start and end of a word, detected by delims 
		wordStart = input.find_first_not_of(" \t\r", wordEnd);	
		wordEnd = input.find_first_of(" \t\r", wordStart);
		
		//wordEnd can be allowed to overflow, but wordStart cannot.
		if(wordStart != std::string::npos) {
			//Incriment word index.
			++wordIndex;
			
			//If this index is the one requested, set output
			if(wordIndex == index) {
				output = input.substr(wordStart, wordEnd - wordStart);
				break;
			}
		}
	} while(wordStart < input.size());	
	
	//If the index could not be found, return an empty string
	if(wordIndex < index) return "";
	return output;
}

std::string CueHandler::padIntStr(const unsigned long val, 
                                  const unsigned int len, const char pad) {
	std::string intStr = std::to_string(val);
	
	//Pad the int string if length wanted is less than current length
	if(len > intStr.length()) {
		unsigned int padDelta = len - intStr.length();
		//Start, length, char
		intStr.insert(0, padDelta, pad);
	}
	
	return intStr;
}

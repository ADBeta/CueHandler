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

#include <iostream>
#include <string>
#include <vector>

#include "TeFiEd.hpp"

#ifndef CUE_HANDLER_H
#define CUE_HANDLER_H

/*** Enums and strings of enums ***********************************************/

//Valid CUE file line types, including INVALID, REM and EMPTY string types.
enum class t_LINE { UNKNOWN, EMPTY, REM, FILE, TRACK, INDEX, INVALID };

//Valid FILE formats. (only binary is supported for now)
enum class t_FILE { UNKNOWN, BINARY, MP3 };
//String of LINE types mapped to enum
extern const std::string t_FILE_str[];

//Valid TRACK types  
/*	AUDIO		Audio/Music (2352 — 588 samples)
	CDG			Karaoke CD+G (2448)
	MODE1/2048	CD-ROM Mode 1 Data (cooked)
	MODE1/2352	CD-ROM Mode 1 Data (raw)
	MODE2/2336	CD-ROM XA Mode 2 Data (form mix)
	MODE2/2352	CD-ROM XA Mode 2 Data (raw)
	CDI/2336	CDI Mode 2 Data
	CDI/2352	CDI Mode 2 Data                                               */
enum class t_TRACK { UNKNOWN, AUDIO, CDG, MODE1_2048, MODE1_2352, MODE2_2336,
                    MODE2_2352, CDI_2336, CDI_2352 };
extern const std::string t_TRACK_str[];

/*** Cue file data structs ****************************************************/
//Grandchild INDEX (3rd level)
struct IndexData {
	unsigned int ID = 0; //Index ID (max 99)
	unsigned long BYTES = 0; //Offset bytes (MM:SS:FF in the .cue file)
};

//Child TRACK (2nd level)
struct TrackData {
	unsigned int ID = 0; //Track ID
	t_TRACK TYPE = t_TRACK::AUDIO; //Which type this track is. Default unknown
	std::vector <IndexData> INDEX; //Vector of INDEXs inside this track (max 99)
};

//Parent FILE (Top level)
struct FileData {
	std::string FILENAME; //Filename of bin file
	t_FILE TYPE = t_FILE::BINARY; //File type, default unknown 
	std::vector <TrackData> TRACK; //Vector of TRACKS in FILE (max 99)
};



/*** CueHandler Class *********************************************************/
class CueHandler {
	public:
	//Constructor takes a filename and passes it to the TeFiEd file object
	//Also creates the data structure array
	CueHandler(const std::string filename);
	
	//Destructor, deletes data structure array and cleans up the TeFiEd object
	~CueHandler();
	
	
	
	//Internal error handler TODO Private. Prints error or waning, or ignores
	//Errors depending on strictLevel
	void handleCueError(const char* msg);
	
	//Force an error and bypass the handler. This is for deep internal errors
	void forceCueError(const char* msg);
	
	//TODO make this private with a setter
	//0: No Strictness
	//1: Warn
	//2: Error & Exit
	unsigned char strictLevel = 0;
	
	
	
	
	
	//TeFiEd Object to hold the .cue file, to skin the text inside
	TeFiEd *cueFile; //TeFiEd text file object //TODO private
	
	
	//Vector of FILEs. Cue Data is stored in this nested vector (INDEX & TRACK)
	std::vector <FileData> FILE;
	
	
	
	
	/*** Cue file data functions **********************************************/
	//Returns the t_LINE of the string passed (whole line from cue file)
	t_LINE LINEStrToType(const std::string lineStr);
	
	//Return the t_FILE of the string passed
	t_FILE FILEStrToType(const std::string fileStr);	
	
	//Returns the t_TRACK of the string passed
	t_TRACK TRACKStrToType(const std::string trackStr);
	
	//Returns the FILE type string from t_FILE_str via enum
	std::string FILETypeToStr(const t_FILE);
	
	//Returns the TRACK type string from t_TRACK_str via enum
	std::string TRACKTypeToStr(const t_TRACK);
	
	////////////////////////////////////////////////////////////////////////////
	//Validation functions. Returns specific error codes
	//Validate an input .cue file string (argv[1])
	int validateCueFilename(std::string);
	
	//Validate FILE
	int validateFILE(const FileData &);
	
	//Validate TRACK
	int validateTRACK(const TrackData &);
	
	//Validate INDEX
	int validateINDEX(const IndexData &);
	
	////////////////////////////////////////////////////////////////////////////
	//Push a new FILE to FILE[]
	void pushFILE(const std::string FN, const t_FILE TYPE);
	
	//Push a new TRACK to the last entry in FILE[]
	void pushTRACK(const unsigned int ID, const t_TRACK TYPE);
	
	//Push a new INDEX to the last entry in FILE[].TRACK[]
	void pushINDEX(const unsigned int ID, const unsigned long BYTES);

	////////////////////////////////////////////////////////////////////////////
	//Converts FileData Object into a string which is a CUE file line
	std::string generateFILELine(const FileData &);
	
	//Converts TrackData Object into a string which is a CUE file line
	std::string generateTRACKLine(const TrackData &);
	
	//Converts IndexData Object into a string which is a CUE file line
	std::string generateINDEXLine(const IndexData &);
	
	/*** Input / Output CUE Handling ******************************************/
	//Gets all the data from a .cue file and populates the FILE vector.
	void getCueData();
	
	//Combines all the cue FILE data (removes seperate files) and pushes it
	//to the CueHandler object passed via reference.
	int combineCueFiles(CueHandler &combined, const std::string outFN,
	                    const std::vector <unsigned long> offsetBytes);
	
	//Output internal .cue data to the cueFile
	void outputCueFile();
	
	//Prints the TRACK and INDEX data of the FileData struct passed.
	void printFILE(FileData &);
	
	
	
	/*** **********************************************************************/
	//Read an existing cue file into the TeFiEd object
	void read();
	
	//Create a new cue file
	void create();

	/*** Writing Functions ****************************************************/
	//Output (overwrite) the TeFiEd object RAM into the file.
	void write();
	
	
	

	/** Helper Functions ******************************************************/
	//Converts a number of bytes into an Audio CD timestamp.
	std::string bytesToTimestamp(const unsigned long bytes);
	
	//Converts an Audio CD timestamp into number of bytes
	unsigned long timestampToBytes(const std::string timestamp);
	
	//Modified from TeFiEd. Returns -index- word in a string
	std::string getWord(const std::string input, unsigned int index);
	
	//Takes an input uint32_t, zero-pads to -pad- then return a string
	std::string padIntStr(const unsigned long val, const unsigned int len = 0,
	                      const char pad = '0');

}; //class CueHandler

#endif

/**
 ******************************************************************
 *
 * Module Name : PiDA.cpp
 *
 * Author/Date : C.B. Lirakis / 05-Mar-19
 *
 * Description : Lassen control entry points. 
 *
 * Restrictions/Limitations : none
 *
 * Change Descriptions : 
 *
 * Classification : Unclassified
 *
 * References : lassen-sk8.pdf - Manual
 *              tsip.pdf - main binary interface control document
 *
 *
 *******************************************************************
 */  
// System includes.
#include <iostream>
using namespace std;

#include <string>
#include <cmath>
#include <ctime>
#include <sys/types.h>
#include <unistd.h>
#include <libconfig.h++>
using namespace libconfig;

#include "TFile.h"
#include "TNtupleD.h"


/// Local Includes.
#include "PiDA.hh"
#include "CLogger.hh"
#include "tools.h"
#include "debug.h"
#include "Geodetic.hh"


PiDA* PiDA::fPiDA;


/**
 ******************************************************************
 *
 * Function Name : PiDA constructor
 *
 * Description : initialize CObject variables
 *
 * Inputs : currently none. 
 *
 * Returns : none
 *
 * Error Conditions :
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
PiDA::PiDA(const char* ConfigFile,const char *Filename) : CObject()
{
    CLogger *Logger = CLogger::GetThis();

    /* Store the this pointer. */
    fPiDA = this;
    SetName("PiDA");
    SetError(); // No error.

    fRun = true;
    f5InputFile = NULL;
    fRootFile   = NULL;
    fNtuple     = NULL;
    fGeodetic   = NULL;
    fGeoCenter  = NULL;

    /* 
     * Set defaults for configuration file. 
     */
    fLogging = true;

    if(!ConfigFile)
    {
	SetError(ENO_FILE,__LINE__);
	return;
    }

    fConfigFileName = strdup(ConfigFile);
    if(!ReadConfiguration())
    {
	SetError(ECONFIG_READ_FAIL,__LINE__);
	return;
    }

    /* USER POST CONFIGURATION STUFF. */

    f5InputFile = NULL;
    OpenInputFile(Filename);
    OpenOutputFile(Filename);

    Logger->Log("# PiDA constructed.\n");

    SET_DEBUG_STACK;
}
/**
 ******************************************************************
 *
 * Function Name : PiDA Destructor
 *
 * Description :
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
PiDA::~PiDA(void)
{
    SET_DEBUG_STACK;
    CLogger *Logger = CLogger::GetThis();

    // Do some other stuff as well. 
    if(!WriteConfiguration())
    {
	SetError(ECONFIG_WRITE_FAIL,__LINE__);
	Logger->LogError(__FILE__,__LINE__, 'W', 
			 "Failed to write config file.\n");
    }
    free(fConfigFileName);

    /* Clean up */
    delete f5InputFile;
    f5InputFile = NULL;

    /* close ntuple */


    fRootFile->Write();
    fRootFile->Close();
    delete fRootFile;
    fRootFile = NULL;

    delete fNtuple;
    fNtuple = NULL;

    delete fGeoCenter;
    delete fGeodetic;

    // Make sure all file streams are closed
    Logger->Log("# PiDA closed.\n");
    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : Do
 *
 * Description : Perform the transfer. 
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
void PiDA::Do(void)
{
    SET_DEBUG_STACK;
    fRun = true;
    Point fXY;

    // Create a vector for holding the data. 
    // How many columns are in the dataset? 
    //size_t NCol = f5InputFile->NVariables();
    const double *var;
    double Lat, Lon;
    // Make this big. 
    double NewVar[32];


    size_t N    = f5InputFile->NEntries();
    size_t NVar =  f5InputFile->NVariables();
    cout << "Processing: " << N << " Entries, with " << NVar 
	 << " Variables" << endl;

    for (size_t i=0 ;i<N; i++)
    {
	if(f5InputFile->DatasetReadRow(i))
	{
	    var = f5InputFile->RowData();

	    memcpy(NewVar, var, NVar*sizeof(double));

	    Lat = var[f5InputFile->IndexFromName("Lat")];
	    Lon = var[f5InputFile->IndexFromName("Lon")];
	    fXY = fGeodetic->ToXY(Lon, Lat, 0.0);
	    NewVar[NVar]   = fXY.X();
	    NewVar[NVar+1] = fXY.Y();
	    NewVar[NVar+2] = fXY.X() - fGeodetic->XY0().X();
	    NewVar[NVar+3] = fXY.Y() - fGeodetic->XY0().Y();
	    fNtuple->Fill(NewVar);
	}
    }

    SET_DEBUG_STACK;
}

/**
 ******************************************************************
 *
 * Function Name : OpenInputFile
 *
 * Description : Open and manage the HDF5 Input file
 *
 * Inputs : none
 *
 * Returns : NONE
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on:  
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
bool PiDA::OpenInputFile(const char *Filename)
{
    SET_DEBUG_STACK;

    // USER TO FILL IN.
    CLogger *pLogger = CLogger::GetThis();
    SET_DEBUG_STACK;

    /*
     * open in read only mode. 
     */
    f5InputFile = new H5Logger( Filename, NULL, 0, true);
    if (f5InputFile->CheckError())
    {
	pLogger->Log("# Failed to open H5 input file: %s\n", Filename);
	delete f5InputFile;
	f5InputFile = NULL;
	return false;
    }

    /* Log that this was done in the local text log file. */
    pLogger->LogTime("Input file name %s \n", Filename);

    //cout << *f5InputFile ;
    return true;
}
/**
 ******************************************************************
 *
 * Function Name : OpenOutputFile
 *
 * Description : 
 *     make a root filename based on input filename
 *     open a TFILE 
 *     create an ntuple to fill. 
 *
 * Inputs :
 *
 * Returns :
 *
 * Error Conditions :
 * 
 * Unit Tested on: 
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
bool PiDA::OpenOutputFile(const char *Filename)
{
    SET_DEBUG_STACK;
    CLogger *pLogger = CLogger::GetThis();
    bool rc = false;
    char name[256]; 
    char *p;

    // Truncate Filename and make a new one
    memset( name, 0, sizeof(name));
    strncpy( name, Filename, sizeof(name));

    // find the dot in the input filename. 
    p = strstr( name, ".h5");
    *p = 0; // null terminate. 
    strcat(name, ".root");
    //cout << " New Filename: " << name << endl;

    /*
     * Initialize Root package.
     * We don't really need to track the return pointer. 
     * We just need to initialize it. 
     */
    ::new TROOT("HDF5","HDF5 Data analysis");

    /* Create disk file */
    fRootFile = new TFile( name, "RECREATE","generic data analysis");

    // Read some interesting data from the HDF5 input file and use it
    // to create the ntuple.

    char Names[2048];
    char tmp[32];
    const char *pName;

    memset( Names, 0, sizeof(Names));

    /*
     * How many columns are in the dataset? 
     * Automagically fill all those. 
     */
    size_t NCol = f5InputFile->NVariables();
    for (uint32_t i = 0;i< NCol; i++)
    {
	pName = f5InputFile->NameFromIndex(i);
	if (i<NCol-1)
	{
	    sprintf(tmp,"%s:",pName);
	    strncat(Names, tmp, sizeof(Names)-strlen(Names));
	}
	else
	{
	    strncat(Names, pName, sizeof(Names)-strlen(Names));
	}
    }
    /*
     * Future.... Add any additional names here. 
     */
    snprintf(tmp, sizeof(tmp), ":X:Y:DX:DY");   // projection
    strncat(Names, tmp, sizeof(Names)-strlen(Names));

    pLogger->Log("# Names: %s\n", Names);


    const char *DataSetName = 
	f5InputFile->HeaderInfo(H5Logger::kDATADESCRIPTOR);
    //cout << DataSetName << endl;

    fNtuple = new TNtupleD("PiDA", DataSetName, Names);

    pLogger->LogTime("Output file name %s \n", name);

    SET_DEBUG_STACK;
    return rc;
}
/**
 ******************************************************************
 *
 * Function Name : ReadConfiguration
 *
 * Description : Open read the configuration file. 
 *
 * Inputs : none
 *
 * Returns : NONE
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on:  
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
bool PiDA::ReadConfiguration(void)
{
    SET_DEBUG_STACK;
    CLogger *Logger = CLogger::GetThis();
    ClearError(__LINE__);
    Config *pCFG = new Config();

    /*
     * Open the configuragtion file. 
     */
    try{
	pCFG->readFile(fConfigFileName);
    }
    catch( const FileIOException &fioex)
    {
	Logger->LogError(__FILE__,__LINE__,'F',
			 "I/O error while reading configuration file.\n");
	return false;
    }
    catch (const ParseException &pex)
    {
	Logger->Log("# Parse error at: %s : %d - %s\n",
		    pex.getFile(), pex.getLine(), pex.getError());
	return false;
    }

    /*
     * Start at the top. 
     */
    const Setting& root = pCFG->getRoot();

    // USER TO FILL IN
    // Output a list of all books in the inventory.
    try
    {
	int    Debug;
	double Lat, Lon;
	/*
	 * index into group PiDA
	 */
	const Setting &MM = root["PiDA"];
	MM.lookupValue("Logging",    fLogging);
	MM.lookupValue("Debug",      Debug);
	MM.lookupValue("Longitude0", Lon);  // Degrees
	MM.lookupValue("Latitude0",  Lat);  // Degrees
	SetDebug(Debug);

	// Make sure that there is no fGeoCenter
	delete fGeoCenter;
	fGeoCenter = new Point( Lon, Lat);
        fGeodetic  = new Geodetic(fGeoCenter->Y(), fGeoCenter->X());
	Logger->Log("# Geodetic center set! %f %f %f %f\n", 
		    Lat, Lon, fGeodetic->XY0().X(), fGeodetic->XY0().Y());

    }
    catch(const SettingNotFoundException &nfex)
    {
	// Ignore.
    }

    delete pCFG;
    pCFG = 0;
    SET_DEBUG_STACK;
    return true;
}

/**
 ******************************************************************
 *
 * Function Name : WriteConfigurationFile
 *
 * Description : Write out final configuration
 *
 * Inputs : none
 *
 * Returns : NONE
 *
 * Error Conditions : NONE
 * 
 * Unit Tested on:  
 *
 * Unit Tested by: CBL
 *
 *
 *******************************************************************
 */
bool PiDA::WriteConfiguration(void)
{
    SET_DEBUG_STACK;
    CLogger *Logger = CLogger::GetThis();
    ClearError(__LINE__);
    Config *pCFG = new Config();

    Setting &root = pCFG->getRoot();

    // USER TO FILL IN
    // Add some settings to the configuration.
    Setting &MM = root.add("PiDA", Setting::TypeGroup);
    MM.add("Debug",     Setting::TypeInt)     = 0;
    MM.add("Logging",   Setting::TypeBoolean) = true;
    // Degrees -
    MM.add("Longitude0",Setting::TypeFloat)   = fGeodetic->CenterLon(); 
    MM.add("Latitude0", Setting::TypeFloat)   = fGeodetic->CenterLat();
    // Write out the new configuration.
    try
    {
	pCFG->writeFile(fConfigFileName);
	Logger->Log("# New configuration successfully written to: %s\n",
		    fConfigFileName);

    }
    catch(const FileIOException &fioex)
    {
	Logger->Log("# I/O error while writing file: %s \n",
		    fConfigFileName);
	delete pCFG;
	return(false);
    }
    delete pCFG;

    SET_DEBUG_STACK;
    return true;
}

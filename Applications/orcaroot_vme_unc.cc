// orcaroot_vme_unc.cc

#include <stdlib.h>
#include <getopt.h>
#include <string>
#include "ORBasicTreeWriter.hh"
#include "ORDataProcManager.hh"
#include "ORFileReader.hh"
#include "ORFileWriter.hh"
#include "ORHistWriter.hh"
#include "ORLogger.hh"
#include "ORSocketReader.hh"
#include "ORHandlerThread.hh"

#include "ORTrigger32ShaperTreeWriter.hh"
#include "ORShaperShaperTreeWriter.hh"

using namespace std;

static const char Usage[] =
  "\n"
  "\n"
  "Usage: orcaroot_vme_unc [options] [input file(s) / socket host:port]\n"
  "\n"
  "The one required argument is either the name of a file to be processed or\n"
  "a host and port of a socket from which to read data. For a file, you may\n"
  "enter a series of files to be processed, or use a wildcard like \"file*.dat\"\n"
  "For a socket, the argument should be formatted as host:port.\n"
  "\n"
  "Available options:\n"
  "  --help : print this message and exit\n"
  "  --verbosity [verbosity] : set the severity/verbosity for the logger.\n"
  "    Choices are: debug, trace, routine, warning, error, and fatal.\n"
  "  --label [label] : use [label] as prefix for root output file name.\n"
  "\n"
  "Example usage:\n"
  "orcaroot run194ecpu\n"
  "  Rootify local file run194ecpu with default verbosity, output file label, etc.\n"
  "orcaroot run*\n"
  "  Same as before, but run over all files beginning with \"run\".\n"
  "orcaroot --verbosity debug --label mylabel run194ecpu\n"
  "  The same, but with example usage of the verbosity and mylabel options.\n"
  "  An output file will be created with name mylabel_run194.root, and lots\n"
  "  of debugging output will appear.\n"
  "orcaroot 128.95.100.213:44666\n"
  "  Rootify orca stream on host 128.95.100.213, port 44666 with default verbosity,\n"
  "  output file label, etc.\n"
  "\n"
  "\n";


int main(int argc, char** argv)
{
  if(argc == 1) {
    ORLog(kError) << "You must supply some options" << endl << Usage;
    return 1;
  }

  static struct option longOptions[] = {
    {"help", no_argument, 0, 'h'},
    {"verbosity", required_argument, 0, 'v'},
    {"label", required_argument, 0, 'l'},
  };

  string label = "OR";
  ORVReader* reader = NULL;

  while(1) {
    char optId = getopt_long(argc, argv, "", longOptions, NULL);
    if(optId == -1) break;
    switch(optId) {
    case('h'): // help
      cout << Usage;
      return 0;
    case('v'): // verbosity
      if(strcmp(optarg, "debug") == 0)
	ORLogger::SetSeverity(ORLogger::kDebug);
      else if(strcmp(optarg, "trace") == 0)
	ORLogger::SetSeverity(ORLogger::kTrace);
      else if(strcmp(optarg, "routine") == 0)
	ORLogger::SetSeverity(ORLogger::kRoutine);
      else if(strcmp(optarg, "warning") == 0)
	ORLogger::SetSeverity(ORLogger::kWarning);
      else if(strcmp(optarg, "error") == 0)
	ORLogger::SetSeverity(ORLogger::kError);
      else if(strcmp(optarg, "fatal") == 0)
	ORLogger::SetSeverity(ORLogger::kFatal);
      else {
	ORLog(kWarning) << "Unknown verbosity setting " << optarg 
			<< "; using kRoutine" << endl;
	ORLogger::SetSeverity(ORLogger::kRoutine);
      }
      break;
    case('l'): // label
      label = optarg;
      break;
    default: // unrecognized option
      ORLog(kError) << Usage;
      return 1;
    }
  }

  if (argc <= optind) {
    ORLog(kError) << "You must supply a filename or socket host:port" << endl
                  << Usage << endl;
    return 1;
  }

  ORHandlerThread* handlerThread = new ORHandlerThread();
  handlerThread->StartThread();

  string readerArg = argv[optind];
  size_t iColon = readerArg.find(":");
  if (iColon == string::npos) {
    if(argc==2){
      TFile *tf=new TFile(argv[1],"raw");
      char buff[4096];
      tf->ReadBuffer(buff,2048);
      string str=buff;
      size_t iCaen965Key = str.find("ORCaen965Model");
      if(iCaen965Key != string::npos){
        ORLog(kRoutine) << "Found key " << "ORCaen965Model" << " ... is a IPE crate file ..." << endl;
      }
      //exit(999);
    }else{
      ORLog(kRoutine) << "More than 1 inputs: Using all implemented decoders   ... " << endl;
    }    

    reader = new ORFileReader;
    for (int i=optind; i<argc; i++) {
      ((ORFileReader*) reader)->AddFileToProcess(argv[i]);
    }
  } else {
    reader = new ORSocketReader(readerArg.substr(0, iColon).c_str(), 
                                atoi(readerArg.substr(iColon+1).c_str()));
  }

  if (!reader->OKToRead()) {
    ORLog(kError) << "Reader couldn't read" << endl;
    return 1;
  }

  ORLog(kRoutine) << "Setting up data processing manager..." << endl;
  ORDataProcManager dataProcManager(reader);

  ORLog(kRoutine) << "Starting output file writer..." << endl;
  ORFileWriter fileWriter(label);
  dataProcManager.AddProcessor(&fileWriter);

  ORLog(kRoutine) << "Saving as a tree..." << endl;

 

  // ORTrigger32ShaperTreeWriter treeWriter;
  // dataProcManager.AddProcessor(&treeWriter);

  ORLog(kRoutine) << "Start processing..." << endl;
  dataProcManager.ProcessDataStream();
  ORLog(kRoutine) << "Finished processing..." << endl;

  delete reader;
  delete handlerThread;

  return 0;
}


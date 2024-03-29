#include "io.hpp"

#include "Config.hpp"
#include "gfx.hpp"

#include <TCanvas.h>
#include <TObjArray.h>
#include <TObjString.h>
#include <TStyle.h>
#include <TList.h>

#include <stdio.h>
#include <assert.h>

io::Logger io::log;

static Config const&cfg=Config::get();


std::string io::shellOutput(const char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while (!feof(pipe)) {
        if (fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result;
}

static void getPathAndFilename(TString fullName,TString& path,TString& fileName);

void io::ensurePathForFile(TString fileName)
{
   TObjArray* tokens = fileName.Tokenize("/");
   TString path("");
   int offset = -1;
   if (fileName[0] == '/') path += "/";
   if (fileName[fileName.Length()-1] == '/') offset = 0;
   for (int i=0; i<tokens->GetEntries() + offset; i++){
      path += ((TObjString*)(tokens->At(i)))->GetString() + "/";
   }
   if (!path.EqualTo("")) system("mkdir -p "+path);
}

/*******************************************************************************
 * class RootFileSaver
 ******************************************************************************/

io::RootFileSaver::RootFileSaver(TString rootFileName,TString internalPath,bool lumiText,bool standardOutputDirectory,bool recreate)
   : fName_(rootFileName)
   // ~, fPath_(CMAKE_SOURCE_DIR+TString::Format("/%s/",cfg.outputDirectory.Data())+fName_)
   , intPath_(internalPath)
   , bLumiText_(lumiText)
{
   TString option="update";
   if (recreate) option="recreate";
   if (standardOutputDirectory) fPath_=TString::Format("%s/",cfg.outputDirectory.Data())+fName_;
   else {
      fPath_=fName_;
   }
   ensurePathForFile(fPath_);
   file_ = new TFile(fPath_,option);
}

io::RootFileSaver::~RootFileSaver()
{
   file_->Close();
   delete file_;
}

void io::RootFileSaver::closeFile() const
{
   file_->Close();
}

TString io::RootFileSaver::getInternalPath() const
{
   return intPath_;
}

TString io::RootFileSaver::getFilePath() const
{
   TString filePath = fPath_;
   return filePath.ReplaceAll(fName_,"");
}

void io::RootFileSaver::save(TObject const &obj, TString name,bool decorate,bool simulation,bool addPDF) const
{  
   if (intPath_!="") name=intPath_+"/"+name;
   if (obj.InheritsFrom(TCanvas::Class())){
      TCanvas &can=*(TCanvas*)&obj;
      can.Update();
      TString title(name);
      title.ReplaceAll("/","_");
      title.ReplaceAll(" ","_");
      title.ReplaceAll(".","_");
      can.SetName(title);
      if (decorate) gfx::decorate(can,simulation,bLumiText_);
      //~ can.SaveAs("test.pdf");
      //~ can.SaveAs("test.root");
      //~ can.SaveAs("test.png");
      
      if (addPDF) {
         TString loc = std::string(fPath_).substr(0, std::string(fPath_).find_last_of("\\/"));
         TString filePath = loc+"/pdf_out/"+name+".pdf";
         TString filePath_eps = loc+"/pdf_out/"+name+".eps";
         ensurePathForFile(filePath);
         can.SaveAs(filePath);
         // ~can.SaveAs(filePath_eps);
      }
   }
   file_->cd();

   TString pathInFile;
   TString nameInFile;
   getPathAndFilename(name,pathInFile,nameInFile);
   if (!file_->cd(pathInFile)){
      io::log<< "directory will be created.";
      file_->mkdir(pathInFile);
      file_->cd(pathInFile);
   }
   obj.Write(nameInFile,TObject::kOverwrite);
   io::log*"Saved '"*name*"' to '"*fPath_<<"'";
}

void io::RootFileSaver::save(gfx::SplitCan &obj, TString name,bool simulation,bool addPDF) const
{
   gfx::setupDrawnAxes(obj);
   obj.cdUp();
   gfx::decorate(obj.pU_,simulation);
   obj.Update();
   // don't let "normal" save-function decorate (no axes in canvas)
   save(obj.can_,name,false,simulation,addPDF);
}

/*******************************************************************************
 * class RootFileReader
 ******************************************************************************/

io::RootFileReader::RootFileReader(TString rootFileName,TString internalPath,bool standardOutputDirectory,bool useDataBasePath)
   : fName_(rootFileName)
   // ~, fPath_(CMAKE_SOURCE_DIR+TString::Format("/%s/",cfg.outputDirectory.Data())+fName_)
   , intPath_(internalPath)
{
   if (standardOutputDirectory) {
      if (useDataBasePath) fPath_=cfg.dataBasePath+"/../output_framework/"+fName_;
      else fPath_=TString::Format("%s/",cfg.outputDirectory.Data())+fName_;
   }
   else fPath_=fName_;
   ensurePathForFile(fPath_);
   // ~file_ = new TFile(fPath_,"read");
   file_ = TFile::Open(fPath_,"read");
   if (file_->IsZombie()) {
      debug_io << "Could not open file: "+fPath_;
      throw;
   }
}

std::vector<TString> io::RootFileReader::listPaths(bool lastFolder) const
{
   std::vector<TString> output = {};
   listAllObjectPaths(file_,intPath_,output,lastFolder);
   return output;
}

void io::RootFileReader::closeFile() const
{
   file_->Close();
}

io::RootFileReader::~RootFileReader()
{
   file_->Close();
   delete file_;
}

/*******************************************************************************
 * class Logger
 ******************************************************************************/

io::Logger::Logger()
   : fName_("")
   , ostream_(&std::cout)
{}
io::Logger::Logger(std::string fileName)
   : fName_(fileName)
   , ostream_(new std::ofstream(CMAKE_SOURCE_DIR+TString::Format("/%s/",cfg.outputDirectory.Data())+fName_,std::ofstream::out))
{
   putVersion();
}
io::Logger::~Logger()
{
   if (fName_.Length()>0){
      static_cast<std::ofstream*>(ostream_)->close();
      delete ostream_;
      io::log*"Written '"*fName_<<"'";
   }
}
void io::Logger::putVersion()
{
   *ostream_<<TString::Format("%%%% %s %s %%%%",cfg.treeVersion.Data(),cfg.gitHash.Data());
   *ostream_<<std::endl;
}


/* STATIC FUNCTIONS */

static void getPathAndFilename(TString fullName,TString& path,TString& fileName)
{
   path="";
   fileName="";
   TObjArray* tokens = fullName.Tokenize("/");
   if (fullName[0] == '/') path += "/";
   int i;
   for (i=0; i<tokens->GetEntries() -1; i++){
      path += ((TObjString*)(tokens->At(i)))->GetString() + "/";
   }
   fileName=((TObjString*)(tokens->At(i)))->GetString();
}

bool io::fileExists(const std::string& name) 
{
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

std::vector<TString> io::listAllObjectPaths(TFile* tf, const TString &path, std::vector<TString> &output, bool lastFolder)
{
   tf->cd(path);
   for (auto key : *(gDirectory->GetListOfKeys())){
      if(key->IsFolder()){
         listAllObjectPaths(tf,path+"/"+key->GetName(),output,lastFolder);
      }
      else{
         if(lastFolder){
            output.push_back(path);
            break;
         }
         else output.push_back(path+"/"+key->GetName());
      }
   }
   return output;
}

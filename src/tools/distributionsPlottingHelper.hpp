#ifndef DISTRIBUTIONSPLOTTINGHELPER_HPP__
#define DISTRIBUTIONSPLOTTINGHELPER_HPP__
#include <TFile.h>
#include <TF1.h>
#include <TF2.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TColor.h>
#include <TGraphAsymmErrors.h>
#include <iomanip>
#include <chrono>
#include <algorithm>

#include "Config.hpp"
#include "tools/hist.hpp"
#include "tools/physics.hpp"
#include "tools/io.hpp"
#include "tools/weighters.hpp"
#include "tools/systematics.hpp"
#include "tools/util.hpp"
#include "tools/tunfoldPlottingHelper.hpp"

class systHists
{
   public:
      systHists(TString const &systematicName, TString filePath, TString const &histPath, std::vector<TString> const &datasets, std::vector<TString> const &datasets_ttBar, std::vector<TString> const &datasets_ttBar2L, std::vector<TString> const &datasets_st);
      void openFile(bool const &standardDict);
      void combineChannel();
      
      Systematic::Systematic systematic_;
      TString filePath_;
      TString histPath_;
      io::RootFileReader* histReader_;
      hist::Histograms<TH1F> hists_;
      std::vector<TString> datasets_;
      std::vector<TString> datasets_ttBar_;
      std::vector<TString> datasets_ttBar2L_;
      std::vector<TString> datasets_st_;
      TString const systematicName_;
      float sf_ = 1.0;
      std::vector<std::string> datasets_SF;   //datasets for which the SF is applied when using the syst.
      bool hasRootFile_ = true;
      bool altSampleType = false;
      bool onlyTTbar = false;
      bool onlyTTbar2L = false;
      bool onlyST = false;
};

namespace distributionsplotting
{
   // struct to combine name of distributions and intended binning
   struct distr {
      TString path;
      TString name;
      float xMin;
      float xMax;
      int nBins;
      std::vector<double> binEdges = {};
   };

   struct distr2D {
      TString path;
      TString name;
      float xMin;
      float xMax;
      int nBinsX;
      float yMin;
      float yMax;
      int nBinsY;
      std::vector<float> binEdgesX = {};
      std::vector<float> binEdgesY = {};
   };
   
   void getSampleVectors(int const &year_int, std::vector<TString> &mcSamples, std::vector<TString> &dataSamples, std::vector<TString> &ttbarSamples, std::vector<TString> &signalSamples, std::vector<TString> &stSamples, std::vector<TString> &bsmSamples, bool const &useDR=false);
   
   void importHists(std::vector<systHists*> &systHists_vec, std::vector<TString> const &samplesToPlot, std::vector<TString> const &mcSamples,
                     std::vector<distr> const &vecDistr, std::vector<distr2D> const &vecDistr2D,  bool const &standardDict=true);
                     
   void add_Categories(TString const path, io::RootFileReader const &reader_hist, TH1F &out_hist);
   
   void combineAllSamples(int const &year_int, hist::Histograms<TH1F>* hs, std::vector<TString> &mcSamples_merged, const bool useDR=false);
   
   void addShifts(const TH1F &tempShift,TH1F* hist_shiftUP,TH1F* hist_shiftDOWN);
   
   std::pair<TH1F*,TH1F*> getTotalSyst(TH1F* const &nominal, std::vector<systHists*> const &systHists_vec, TString const loc, TString const sample="", bool run2Combi=false, bool envelope=false);
   
   void printUnc(TString name, const float &down, const float &up, const float &nominal);
   
   void printTotalYields(hist::Histograms<TH1F>* hs, std::vector<std::vector<systHists*>> const &systHists_vec, const std::vector<TString> &mcSamples_merged);
   
   void printUncBreakDown(hist::Histograms<TH1F>* hs, std::vector<systHists*> &systHists_vec, const std::vector<TString> &mcSamples);
   
   void printShiftBySample(hist::Histograms<TH1F>* hs, std::vector<systHists*> &systHists_vec, const std::vector<TString> &mcSamples);
   
   void fixAxis2D(TAxis* axis, const int &nBinsY);
   
   void drawVertLines2D(std::vector<float> const &binEdgesY, float const &lowerEnd, float const &upperEnd,bool text);
   
   void plotHistograms(TString const &sPresel, TString const &sVar, hist::Histograms<TH1F>* const &hs, std::vector<TString> const &mcSamples_merged, 
                     std::map<const TString,Color_t> const & colormap, std::map<const TString,TString> const & printNameMap, std::vector<std::vector<systHists*>> const &systHists_vec, io::RootFileSaver const &saver, bool plotBSM=false, bool is2D=false, const std::vector<float> &binEdgesY={});
                     
   void getCombinedDistributions(hist::Histograms<TH1F> &hs_combined, std::vector<hist::Histograms<TH1F>*> const &hs_vec, std::vector<TString> const &mcSamples_merged);
   
   std::pair<TH1F*,TH1F*> getTotalSystCombined(std::vector<std::vector<systHists*>> const &systHists_vec_all, TString const loc);
   
   TString getPrintName(TString const& sampleName);
   
} // namespace distributionsplotting

#endif /* DISTRIBUTIONSPLOTTINGHELPER_HPP__ */

//Script to plot the result of TUnfold_unfolding
//This script uses a fake bin to take into account fakes
#include <iostream>
#include <cmath>
#include <map>
#include <TMath.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TGraph.h>
#include <TFile.h>
#include <TH1.h>
#include "TUnfoldDensity.h"
#include <TRandom3.h>
#include <TProfile.h>
#include <TVectorD.h>


#include "Config.hpp"
#include "tools/hist.hpp"
#include "tools/physics.hpp"
#include "tools/io.hpp"
#include "tools/weighters.hpp"

using namespace std;

Config const &cfg=Config::get();

extern "C"
void run()
{
   // unfolded sample
   TString sample="MadGraph";
   // ~TString sample="dilepton";
   // ~TString sample="";
   
   // response sample
   // ~TString sample_response="MadGraph";
   TString sample_response="dilepton";
   // ~TString sample_response="";
   
   // include signal to pseudo data
   // ~bool withBSM = true;
   bool withBSM = false;
   
   //Use scale factor
   bool withScaleFactor = false;
   // ~bool withScaleFactor = true;
   
   //==============================================
   // step 1 : open output file
   io::RootFileSaver saver(TString::Format("plots%.1f.root",cfg.processFraction*100),TString::Format(!withScaleFactor ? "TUnfold_plotting_fakeBin_%.1f" : "TUnfold_plotting_fakeBin_SF_%.1f",cfg.processFraction*100));

   //==============================================
   // step 2 : read binning schemes and input histograms
   io::RootFileReader histReader(TString::Format(!withScaleFactor ? "TUnfold_fakeBin_%.1f.root" : "TUnfold_fakeBin_SF91_%.1f.root",cfg.processFraction*100));
   // ~io::RootFileReader histReader(TString::Format("TUnfold_SF91_%.1f.root",cfg.processFraction*100));
   TString input_loc="TUnfold_binning_"+sample+"_"+sample_response;
   TString input_loc_result="TUnfold_results_"+sample+"_"+sample_response;
   if (withBSM) {
      input_loc+="_BSM";
      input_loc_result+="_BSM";
   }

   TUnfoldBinning *detectorBinning=histReader.read<TUnfoldBinning>(input_loc+"/detector_binning");
   TUnfoldBinning *generatorBinning=histReader.read<TUnfoldBinning>(input_loc+"/generator_binning");

   if((!detectorBinning)||(!generatorBinning)) {
      cout<<"problem to read binning schemes\n";
   }

   // read histograms
   TH1F *realDis=histReader.read<TH1F>(input_loc+"/histDataTruth");
   TH1F *realDis_response=histReader.read<TH1F>(input_loc+"/histMCGenRec_projX");
   TH1F *unfolded=histReader.read<TH1F>(input_loc_result+"/hist_unfoldedResult");
   TH2F *corrMatrix=histReader.read<TH2F>(input_loc_result+"/corr_matrix");

   if((!realDis)||(!realDis_response)||(!unfolded)) {
      cout<<"problem to read input histograms\n";
   }

   //========================
   // Step 3: plotting
   
   gfx::SplitCan can;
   can.can_.SetWindowSize(1800,600);
   can.pU_.SetLogy();
   can.pU_.cd();  //Use upper part of canvas
   
   
   //Initialize proper binning for plotting
   TVectorD binning_met(*(generatorBinning->FindNode("signal")->GetDistributionBinning(0)));
   TVectorD binning_phi(*(generatorBinning->FindNode("signal")->GetDistributionBinning(1)));
   
   int num_bins = binning_met.GetNoElements()*(binning_phi.GetNoElements()-1)+1;
   int num_bins_met = binning_met.GetNoElements();
   
   binning_met.ResizeTo(binning_met.GetNoElements()+1);
   binning_met[binning_met.GetNoElements()-1] = 400;  //Plotting end for overflow bin
   
   Double_t xbins[num_bins+1];
   xbins[0] = -30;   //Plotting start for fake bin
   xbins[1] = 0;   //Plotting start for fake bin
   
   int phi_bin = 0;
   for (int i=0; i<(num_bins-1); i++)   {
      xbins[i+2] = binning_met[i%num_bins_met+1]+phi_bin*400;
      if (i%num_bins_met==num_bins_met-1) phi_bin++;
   }
   
   unfolded->SetBins(num_bins,xbins);
   realDis->SetBins(num_bins,xbins);
   realDis_response->SetBins(num_bins,xbins);
   for (int i=2; i<=num_bins; i++) {  //Set proper label for x axis
      int bin_label_no = (i-2)%num_bins_met+1;
      TString label;
      if (bin_label_no == num_bins_met) label = ">"+std::to_string((int)binning_met[bin_label_no-1]);
      else label = std::to_string((int)binning_met[bin_label_no-1])+"-"+std::to_string((int)binning_met[bin_label_no]);
      unfolded->GetXaxis()->SetBinLabel(i,label);
      realDis->GetXaxis()->SetBinLabel(i,label);
   }
   unfolded->GetXaxis()->SetTickLength(0.);
   unfolded->GetYaxis()->SetTickLength(0.008);
   unfolded->GetXaxis()->SetTitleOffset(1.5);
   unfolded->GetYaxis()->SetTitleOffset(0.8);
   unfolded->GetXaxis()->CenterLabels(false);
   realDis->GetXaxis()->SetBinLabel(1,"fakes");   //End binning initializing
   
   unfolded->LabelsOption("v");
   realDis->LabelsOption("v");
   unfolded->SetMaximum(4*unfolded->GetMaximum());
   unfolded->SetMinimum(2);
   unfolded->SetLineColor(kBlack);
   unfolded->SetTitle(";p_{T}^{#nu#nu} (GeV);arbitrary unit");
   unfolded->SetStats(false);
   realDis->SetTitle(";p_{T}^{#nu#nu} (GeV);arbitrary unit");
   realDis->SetStats(false);
   
   unfolded->Draw("pe1");  //Draw into current canvas 
   realDis->SetLineColor(kRed-6);
   realDis->SetFillColor(kRed-6);
   realDis->Draw("hist same");   //Dra into current canvas (axis are not drawn again due to "same")
   
   realDis_response->SetLineColor(kBlue);
   realDis_response->SetFillColor(kBlue);
   realDis_response->Draw("hist same");
   
   //Draw vertical lines and binning ranges for deltaPhi
   TLine * aline = new TLine();
   TLatex * atext = new TLatex();
   atext->SetTextSize(0.03);
   aline->SetLineWidth(2);
   aline->DrawLine(0,2,0,unfolded->GetMaximum());
   aline->DrawLine(800,2,800,unfolded->GetMaximum());
   aline->DrawLine(400,2,400,unfolded->GetMaximum());
   aline->DrawLine(800,2,800,unfolded->GetMaximum());
   atext->DrawLatex(75,0.5*unfolded->GetMaximum(),"0<|#Delta#phi|(p_{T}^{#nu#nu},nearest l)<0.7");
   atext->DrawLatex(475,0.5*unfolded->GetMaximum(),"0.7<|#Delta#phi|(p_{T}^{#nu#nu},nearest l)<1.4");
   atext->DrawLatex(875,0.5*unfolded->GetMaximum(),"1.4<|#Delta#phi|(p_{T}^{#nu#nu},nearest l)<3.14");
   
   //Dra legend
   gfx::LegendEntries legE;
   legE.append(*unfolded,"Unfolded","pe");
   legE.append(*realDis,"MC true ttbar","l");
   legE.append(*realDis_response,"MC true ttbar (response)","l");
   TLegend leg=legE.buildLegend(.15,.45,0.3,.6,1);
   leg.SetTextSize(0.035);
   leg.Draw();
   
   unfolded->Draw("axis same");
   
   if (withScaleFactor) {
      atext->DrawLatex(75,10,"With ScaleFactor");
   }
   
   //Change to lower part of canvas
   can.pL_.cd();
   can.pL_.SetBottomMargin(0.45);
   can.pL_.SetTickx(0);
   TH1F ratio=hist::getRatio(*realDis,*unfolded,"ratio",hist::NOERR);   //Get Ratio between unfolded and true hists
   ratio.SetMaximum(1.1);
   ratio.SetMinimum(0.9);
   ratio.SetLineColor(kRed-6);
   ratio.SetMarkerColor(kRed-6);
   ratio.GetYaxis()->SetTitleOffset(0.3);
   ratio.GetXaxis()->SetTitleOffset(1.3);
   ratio.GetXaxis()->SetTickLength(0.);
   ratio.Draw();
   
   TH1F ratio_response=hist::getRatio(*realDis_response,*unfolded,"ratio",hist::NOERR);
   ratio_response.SetLineColor(kBlue);
   ratio_response.SetMarkerColor(kBlue);
   ratio_response.Draw("same");
   
   TH1F uncertainty_unfolded=hist::getRatio(*unfolded,*unfolded,"ratio",hist::ONLY1);  //Get Ratio between unfolded and unfolded to draw unfolded uncertainty around 1
   uncertainty_unfolded.SetMarkerSize(0);
   uncertainty_unfolded.Draw("same");
   
   // ~aline->SetLineStyle(2);
   aline->DrawLine(0,ratio.GetMinimum(),0,ratio.GetMaximum());
   aline->DrawLine(800,ratio.GetMinimum(),800,ratio.GetMaximum());
   aline->DrawLine(400,ratio.GetMinimum(),400,ratio.GetMaximum());
   aline->DrawLine(800,ratio.GetMinimum(),800,ratio.GetMaximum());
   
   //Plot correlation matrix
   TCanvas can2D;
   can2D.cd();
   
   gPad->SetRightMargin(0.2);
   corrMatrix->SetStats(0);
   corrMatrix->SetTitle("");
   corrMatrix->GetYaxis()->SetTitleOffset(0.6);
   corrMatrix->GetYaxis()->SetTitle("BinNo");
   corrMatrix->GetXaxis()->SetTitle("BinNo");
   corrMatrix->GetZaxis()->SetTitleOffset(1.2);
   corrMatrix->GetZaxis()->SetLabelOffset(0.01);
   corrMatrix->SetMarkerColor(kRed);
   corrMatrix->Draw("hcolz text");
   
   
   //===========================
   // Step 4: save plot
   TString saveName=sample+"_"+sample_response;
   TString saveName2D="correlations/"+sample+"_"+sample_response;
   if (withBSM) saveName+="_BSM";
   saver.save(can,saveName);
   saver.save(can2D,saveName2D);

}

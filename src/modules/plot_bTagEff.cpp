//Script to plot trigger efficiency studies

#include "Config.hpp"
#include "tools/hist.hpp"
#include "tools/physics.hpp"
#include "tools/io.hpp"
#include "tools/weighters.hpp"
#include "tools/systematics.hpp"

#include <TFile.h>
#include <TF1.h>
#include <TF2.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TColor.h>
#include <TEfficiency.h>

Config const &cfg=Config::get();

extern "C"
void run()
{
   std::vector<TString> systToPlot = {"Nominal","JESTotal_UP","JESTotal_DOWN","JER_UP","JER_DOWN","UETUNE_UP","UETUNE_DOWN",
                                       "MATCH_UP","MATCH_DOWN","ERDON","CR1","CR2","MTOP169p5","MTOP175p5","TOP_PT",
                                       "MESCALE_UP","MESCALE_DOWN","MERENSCALE_UP","MERENSCALE_DOWN","MEFACSCALE_UP","MEFACSCALE_DOWN",
                                       "PSISRSCALE_UP","PSISRSCALE_DOWN","PSFSRSCALE_UP","PSFSRSCALE_DOWN","BFRAG_UP","BFRAG_DOWN",
                                       "BSEMILEP_UP","BSEMILEP_DOWN","PDF_ALPHAS_UP","PDF_ALPHAS_DOWN",
                                       
                                       "JESAbsoluteMPFBias_UP","JESAbsoluteMPFBias_DOWN","JESAbsoluteScale_UP","JESAbsoluteScale_DOWN",
                                       "JESAbsoluteStat_UP","JESAbsoluteStat_DOWN","JESFlavorQCD_UP","JESFlavorQCD_DOWN","JESFragmentation_UP",
                                       "JESFragmentation_DOWN","JESPileUpDataMC_UP","JESPileUpDataMC_DOWN","JESPileUpPtBB_UP","JESPileUpPtBB_DOWN",
                                       "JESPileUpPtEC1_UP","JESPileUpPtEC1_DOWN","JESPileUpPtRef_UP","JESPileUpPtRef_DOWN","JESRelativeBal_UP",
                                       "JESRelativeBal_DOWN","JESRelativeFSR_UP","JESRelativeFSR_DOWN","JESRelativeJEREC1_UP","JESRelativeJEREC1_DOWN",
                                       "JESRelativePtBB_UP","JESRelativePtBB_DOWN","JESRelativePtEC1_UP","JESRelativePtEC1_DOWN","JESRelativeSample_UP",
                                       "JESRelativeSample_DOWN","JESRelativeStatEC_UP","JESRelativeStatEC_DOWN","JESRelativeStatFSR_UP","JESRelativeStatFSR_DOWN",
                                       "JESSinglePionECAL_UP","JESSinglePionECAL_DOWN","JESSinglePionHCAL_UP","JESSinglePionHCAL_DOWN","JESTimePtEta_UP",
                                       "JESTimePtEta_DOWN",
                                       
                                       "JESFlavorRealistic_UP","JESFlavorRealistic_DOWN","JESFlavorPureGluon_UP","JESFlavorPureGluon_DOWN","JESFlavorPureQuark_UP",
                                       "JESFlavorPureQuark_DOWN","JESFlavorPureCharm_UP","JESFlavorPureCharm_DOWN","JESFlavorPureBottom_UP","JESFlavorPureBottom_DOWN"
                                    };
   // ~std::vector<TString> systToPlot = {"Nominal"};
   
   for (TString systName : systToPlot){
      std::cout<<"---------------"<<systName<<"----------------"<<std::endl;
      Systematic::Systematic currentSystematic(systName);
      
      //Set correct sampleName (different for some systematics)
      TString sampleName ="TTbar_diLepton";
      if(std::find(Systematic::altSampleTypes.begin(), Systematic::altSampleTypes.end(), currentSystematic.type()) != Systematic::altSampleTypes.end()){
         sampleName = sampleName+"_"+currentSystematic.name();
      }
      
      TString inputLoc = TString::Format("bTagEff/%s/%s_merged_%s.root",currentSystematic.name().Data(),sampleName.Data(),cfg.treeVersion.Data());
      io::RootFileReader histReader(inputLoc,TString::Format("bTagEff%.1f",cfg.processFraction*100));
      io::RootFileSaver saver(TString::Format("bTagEff/plots_bTagEff%.1f.root",cfg.processFraction*100),TString::Format("%s/",currentSystematic.name().Data()));
      io::RootFileSaver histSaver(TString::Format("data/bTagEff/%s.root",currentSystematic.name().Data()),"");
      
      //Plot 2D bTag Eff.
      TCanvas can;
      can.SetLogx();
      for(TString channel:{"ee","mumu","emu"}){
         for(TString flavor:{"B","C","Light"}){
            for(TString tagger:{"DeepJet_loose","DeepCSV_loose"}){
               TH2F* all=histReader.read<TH2F>("baseline/"+channel+"/"+flavor+"_all/"+sampleName);
               TH2F* tagged=histReader.read<TH2F>("baseline/"+channel+"/"+flavor+"_"+tagger+"/"+sampleName);
               
               std::vector<float> Edges_eta={0,0.5,1.0,2.4};
               std::vector<float> Edges_pT={30,50,70,100,150,200,300,1000};
               *all=hist::rebinned(*all,Edges_pT,Edges_eta);
               *tagged=hist::rebinned(*tagged,Edges_pT,Edges_eta);

               TEfficiency eff(*tagged,*all);   // Will complain about weights due to rebinning using only float precision
               eff.SetUseWeightedEvents(kFALSE);
               TH2* eff_hist = eff.CreateHistogram();
               
               for(int i=0; i<=eff_hist->GetNbinsX();i++){
                  for(int j=0; j<=eff_hist->GetNbinsY();j++){
                     eff_hist->SetBinError(i,j,eff.GetEfficiencyErrorUp(eff.GetGlobalBin(i,j)));
                  }
               }
               TString cat;
               if (channel.Contains("ee")) cat="ee";
               else if (channel.Contains("emu")) cat="e#mu";
               else if (channel.Contains("mumu")) cat="#mu#mu";
               TLatex label=gfx::cornerLabel(cat,1);
               
               gPad->SetRightMargin(0.2);
               gPad->SetLeftMargin(0.13);
               
               eff_hist->GetYaxis()->SetTitleOffset(1.0);
               eff_hist->GetZaxis()->SetLabelOffset(0.02);
               if(flavor=="B"){
                  eff_hist->SetMaximum(1.0);
                  eff_hist->SetMinimum(0.6);
               }
               else if(flavor=="C"){
                  eff_hist->SetMaximum(0.7);
                  eff_hist->SetMinimum(0.1);
               }
               else {
                  eff_hist->SetMaximum(0.5);
                  eff_hist->SetMinimum(0.0);
               }
               eff_hist->SetMarkerSize(1.2);
               eff_hist->Draw("colz text e");
               label.Draw();
               saver.save(can,"baseline/"+channel+"/"+flavor+"_"+tagger,true,true,true);
               histSaver.save(*eff_hist,channel+"/"+flavor+"_"+tagger,false,false);
            }
         }
      }
   }
}

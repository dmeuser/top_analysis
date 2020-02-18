//Script to define unfolding binning and generate distributions used for the unfolding based on TUnfolding classes (Following TUnfold examples)

#include <iostream>
#include <map>
#include <cmath>
#include <TMath.h>
#include <TFile.h>
#include <TTree.h>
#include <TH1.h>
#include "TUnfoldBinning.h"

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
   std::cout<<"---------------------------------------"<<std::endl;
   std::cout<<"Still have to take weights into account (especially how to handle SF weight, which gets important using real data)"<<std::endl;
   std::cout<<"---------------------------------------"<<std::endl;
   
   // unfolded sample
   TString sample="MadGraph";
   // ~TString sample="dilepton";
   
   // response sample
   // ~TString sample_response="MadGraph";
   TString sample_response="dilepton";
   
   // number of met and phi bins
   int NBIN_MET_FINE=7;
   int NBIN_PHI_FINE=6;

   int NBIN_MET_COARSE=3;
   int NBIN_PHI_COARSE=3;

   // met binning (last bin is overflow)
   Double_t metBinsFine[NBIN_MET_FINE+1]={0,30,60,90,120,175,230,315};
   // ~Double_t metBinsCoarse[NBIN_MET_COARSE+1]={0,60,120,230,400};
   Double_t metBinsCoarse[NBIN_MET_COARSE+1]={0,60,120,230};
   // phi binning
   Double_t phiBinsFine[NBIN_PHI_FINE+1]={0,0.35,0.7,1.05,1.4,2.27,3.141};
   Double_t phiBinsCoarse[NBIN_PHI_COARSE+1]={0,0.7,1.4,3.141};
   //=======================================================================
   // detector level binning scheme

   TUnfoldBinning *detectorBinning=new TUnfoldBinning("detector");
   TUnfoldBinning *detectorDistribution=detectorBinning->AddBinning("detectordistribution");
   detectorDistribution->AddAxis("met",NBIN_MET_FINE,metBinsFine,
                                 false, // no underflow bin (not reconstructed)
                                 true // overflow bin
                                 );
   detectorDistribution->AddAxis("phi",NBIN_PHI_FINE,phiBinsFine,
                                 false, // no underflow bin (not reconstructed)
                                 false // no overflow bin (not reconstructed)
                                 );

   //=======================================================================
   // generator level binning
   TUnfoldBinning *generatorBinning=new TUnfoldBinning("generator");

   // signal distribution is measured with coarse binning
   // fake bin is used to check
   // what happens outside the phase-space
   TUnfoldBinning *fakeBinning = generatorBinning->AddBinning("fake",1);
   TUnfoldBinning *signalBinning = generatorBinning->AddBinning("signal");
   signalBinning->AddAxis("metnunugen",NBIN_MET_COARSE,metBinsCoarse,
                           false, // underflow bin
                           true // overflow bin
                           );
   signalBinning->AddAxis("phigen",NBIN_PHI_COARSE,phiBinsCoarse,
                           false, // underflow bin
                           false // overflow bin
                           );
  
  

   // switch on histogram errors
   TH1::SetDefaultSumw2();

   //=======================================================
   // Step 1: open file to save histograms and binning schemes

   io::RootFileSaver saver(TString::Format("TUnfold%.1f.root",cfg.processFraction*100),"TUnfold_binning_"+sample+"_"+sample_response);
   // ~io::RootFileSaver saver(TString::Format("TUnfold_SF91_%.1f.root",cfg.processFraction*100),"TUnfold_binning_"+sample+"_"+sample_response);

   //=======================================================
   // Step 2: save binning to output file

   saver.save(*detectorBinning,"detector_binning");
   saver.save(*generatorBinning,"generator_binning");

   //=======================================================
   // Step 3: book and fill data histograms

   Float_t phiRec,metRec,phiGen,metGen,mcWeight,recoWeight;
   UInt_t genDecayMode;
   // ~Int_t istriggered,issignal;

   TH1 *histDataReco=detectorBinning->CreateHistogram("histDataReco");
   TH1 *histDataTruth=generatorBinning->CreateHistogram("histDataTruth");

   TFile *dataFile=new TFile("/net/data_cms1b/user/dmeuser/top_analysis/output/ttbar_res100.0.root");
   // ~TFile *dataFile=new TFile("/net/data_cms1b/user/dmeuser/top_analysis/output/ttbar_res_SF0.910000100.0.root");
   TTree *dataTree=(TTree *) dataFile->Get("ttbar_res100.0/ttbar_res_"+sample);
   

   if(!dataTree) {
      cout<<"could not read 'data' tree\n";
   }
  
   dataTree->ResetBranchAddresses();
   dataTree->SetBranchAddress("Phi_rec",&phiRec);
   dataTree->SetBranchAddress("MET",&metRec);
   // for real data, only the triggered events are available
   // ~dataTree->SetBranchAddress("istriggered",&istriggered);
   // data truth parameters
   dataTree->SetBranchAddress("Phi_NuNu",&phiGen);
   dataTree->SetBranchAddress("PtNuNu",&metGen);
   dataTree->SetBranchAddress("genDecayMode",&genDecayMode);
   dataTree->SetBranchAddress("N",&mcWeight);
   dataTree->SetBranchAddress("SF",&recoWeight);
   // ~dataTree->SetBranchAddress("issignal",&issignal);


   cout<<"loop over data events\n";

   for(Int_t ievent=0;ievent<dataTree->GetEntriesFast();ievent++) {
      if(dataTree->GetEntry(ievent)<=0) break;

      //only bin to bin migration
      if(metRec<0 || genDecayMode>3 || metGen<0) continue;
      
      //remove tau events, which are not reconstructed (no gen match, no reco match, should be included into distributions.cpp!!!!)
      if(metRec<0 && genDecayMode>3) continue;

      // fill histogram with data truth parameters
      Int_t genbinNumber=signalBinning->GetGlobalBinNumber(metGen,phiGen);
      if (metGen<0 || genDecayMode>3) genbinNumber=fakeBinning->GetStartBin();
      histDataTruth->Fill(genbinNumber,mcWeight);

      // fill histogram with reconstructed quantities
      if (metRec<0) continue;   //events that are not reconstructed
      Int_t binNumber=detectorDistribution->GetGlobalBinNumber(metRec,phiRec);
      histDataReco->Fill(binNumber,mcWeight);
   }
   
   // set reco bin error to error expected in data
   for(int i=0;i<=histDataReco->GetNbinsX()+1;i++) {
      histDataReco->SetBinError(i,sqrt(histDataReco->GetBinContent(i)));
   }
  
   saver.save(*histDataReco,"histDataReco");
   saver.save(*histDataTruth,"histDataTruth");

   delete dataTree;

   //=======================================================
   // Step 4: book and fill histogram of migrations
   //         it receives events from both signal MC and background MC (right now only signal MC)

   TH2 *histMCGenRec=TUnfoldBinning::CreateHistogramOfMigrations(generatorBinning,detectorBinning,"histMCGenRec");
   TH2 *histMCGenRec_purity=TUnfoldBinning::CreateHistogramOfMigrations(generatorBinning,generatorBinning,"histMCGenRec_purity");

   TTree *signalTree=(TTree *) dataFile->Get("ttbar_res100.0/ttbar_res_"+sample_response);

   if(!signalTree) {
      cout<<"could not read 'signal' tree\n";
   }
  
   signalTree->ResetBranchAddresses();
   signalTree->SetBranchAddress("Phi_rec",&phiRec);
   signalTree->SetBranchAddress("MET",&metRec);
   // ~signalTree->SetBranchAddress("istriggered",&istriggered);
   signalTree->SetBranchAddress("Phi_NuNu",&phiGen);
   signalTree->SetBranchAddress("PtNuNu",&metGen);
   signalTree->SetBranchAddress("genDecayMode",&genDecayMode);
   signalTree->SetBranchAddress("N",&mcWeight);
   signalTree->SetBranchAddress("SF",&recoWeight);

   cout<<"loop over MC signal events\n";
  
   for(Int_t ievent=0;ievent<signalTree->GetEntriesFast();ievent++) {
      if(signalTree->GetEntry(ievent)<=0) break;
      
      //only bin to bin migration
      if(metRec<0 || genDecayMode>3 || metGen<0) continue;

      //remove tau events, which are not reconstructed (no gen match, no reco match, should be included into distributions.cpp!!!!)
      if(metRec<0 && genDecayMode>3) continue;

      // bin number on generator level for signal
      Int_t genBin=signalBinning->GetGlobalBinNumber(metGen,phiGen);
      if (metGen<0 || genDecayMode>3) genBin=fakeBinning->GetStartBin();

      // bin number on reconstructed level
      // bin number 0 corresponds to non-reconstructed events
      Int_t recBin=0;
      Int_t recBin_purity=0;
      recBin=detectorDistribution->GetGlobalBinNumber(metRec,phiRec);
      recBin_purity=signalBinning->GetGlobalBinNumber(metRec,phiRec);

      histMCGenRec->Fill(genBin,recBin,mcWeight);
      histMCGenRec_purity->Fill(genBin,recBin_purity,mcWeight);
      
      // ~if(genBin==1 && recBin==0) std::cout<<metGen<<"   "<<phiGen<<"   "<<metRec<<"   "<<phiRec<<std::endl;

      /* Still have to fill with weights
      hist_migration_MC[k]->Fill(genBin,recoBin,weightRec);
      // count fraction of events which have a different weight on truth and reco (in reco underflow bin)
      // this is required for TUnfold to function properly
      hist_migration_MC[k]->Fill(genBin,0.,weightGen-weightRec);
      */
   }
  
   saver.save(*histMCGenRec,"histMCGenRec");
   saver.save(*(histMCGenRec->ProjectionX()),"histMCGenRec_projX");
   saver.save(*(histMCGenRec->ProjectionY()),"histMCGenRec_projY");

   delete signalTree;

   /*
   TFile *bgrFile=new TFile("testUnfold5_background.root");
   TTree *bgrTree=(TTree *) bgrFile->Get("background");

   if(!bgrTree) {
     cout<<"could not read 'background' tree\n";
   }

   bgrTree->ResetBranchAddresses();
   bgrTree->SetBranchAddress("phirec",&phiRec);
   bgrTree->SetBranchAddress("metrec",&metRec);
   bgrTree->SetBranchAddress("discr",&discr);
   bgrTree->SetBranchAddress("istriggered",&istriggered);
   bgrTree->SetBranchStatus("*",1);

   cout<<"loop over MC background events\n";

   for(Int_t ievent=0;ievent<bgrTree->GetEntriesFast();ievent++) {
      if(bgrTree->GetEntry(ievent)<=0) break;

      // here, for background only reconstructed quantities are known
      // and only the reconstructed events are relevant
      if(istriggered) {
         // bin number on generator level for background
         Int_t genBin=bgrBinning->GetGlobalBinNumber(metRec,phiRec);
         // bin number on reconstructed level
         Int_t recBin=detectordistribution->GetGlobalBinNumber
            (metRec,phiRec,discr);
         histMCGenRec->Fill(genBin,recBin);
      }
   }

   delete bgrTree;
   delete bgrFile;
   */
  
   std::cout<<histMCGenRec->GetBinContent(1,0)<<std::endl;
   std::cout<<histMCGenRec->GetBinContent(0,0)<<std::endl;
   std::cout<<histMCGenRec->GetBinContent(0,1)<<std::endl;
   //efficiency
   TH1 *hist_efficiency=generatorBinning->CreateHistogram("efficiency");
   for(int binGen=0;binGen<=histMCGenRec->GetNbinsX()+1;binGen++) {
      double sum0=0.;
      double sum1=0.;
      for(int binRec=0;binRec<=histMCGenRec->GetNbinsY()+1;binRec++) {
         double c=histMCGenRec->GetBinContent(binGen,binRec);
         sum0+=c;
         // ~if((binRec>0)&&(binRec<=histMCGenRec->GetNbinsY())) sum1+=c;
         if((binRec>0)&&(binRec<=histMCGenRec->GetNbinsY()+1)) sum1+=c;
      } 
      if(sum0>0.0) hist_efficiency->SetBinContent(binGen,sum1/sum0);
   }
   saver.save(*hist_efficiency,"hist_efficiency");
   
   // calculate bin purities for the three binning schemes
   TH1 *hist_purity=generatorBinning->CreateHistogram("purity");
   for(int binRec=0;binRec<=hist_purity->GetNbinsX()+1;binRec++) {
      double sum=0.;
      // ~for(int binGen=0;binGen<=hist_purity->GetNbinsX()+1;binGen++) {
      for(int binGen=2;binGen<=hist_purity->GetNbinsX()+1;binGen++) {
         sum += histMCGenRec_purity->GetBinContent(binGen,binRec);
      }
      double p=0.;
      if(sum>0.0) {
         p=histMCGenRec_purity->GetBinContent(binRec,binRec)/sum;
      }
      hist_purity->SetBinContent(binRec,p);
   }
   saver.save(*hist_purity,"hist_purity");

}

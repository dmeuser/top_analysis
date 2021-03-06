//Script to extract distributions separetely for fake and nofake events
#include "Config.hpp"
#include "tools/hist.hpp"
#include "tools/physics.hpp"
#include "tools/io.hpp"
#include "tools/weighters.hpp"
#include "tools/selection.hpp"

#include <TFile.h>
#include <TGraphErrors.h>
#include <TTreeReader.h>
#include <TF1.h>
#include <TVector3.h>
#include <TMath.h>
#include <TStyle.h>
#include <TNtuple.h>
#include <iostream>
#include <fstream>

Config const &cfg=Config::get();

extern "C"
void run()
{
   // ~std::vector<TString> vsDatasubsets({"TT_TuneCUETP8M2T4_13TeV-powheg-pythia8_merged"});
   std::vector<TString> vsDatasubsets({"TTTo2L2Nu_TuneCP5_PSweights_13TeV-powheg-pythia8"});
   
   hist::Histograms<TH1F> hs(vsDatasubsets);    //Define histograms in the following
   for (TString selection:{"Fakes/","NoFakes/","Taus/"}){
      hs.addHist(selection+"met"   ,";%MET;EventsBIN"           ,100,0,600);
      hs.addHist(selection+"met1000"   ,";%MET;EventsBIN"           ,100,0,1000);
      hs.addHist(selection+"pTlep1"   ,";%pTl1;EventsBIN"           ,100,0,600);
      hs.addHist(selection+"pTlep2"   ,";%pTl2;EventsBIN"           ,100,0,600);
      hs.addHist(selection+"etalep1"   ,";|#eta_{l1}|;EventsBIN"           ,100,0,2.5);
      hs.addHist(selection+"etalep2"   ,";|#eta_{l2}|;EventsBIN"           ,100,0,2.5);
      hs.addHist(selection+"pTbJet"   ,";p_{T}^{b}(GeV);EventsBIN"           ,100,0,600);
      hs.addHist(selection+"etabJet"   ,";|#eta_{b}|;EventsBIN"           ,100,0,2.5);
      hs.addHist(selection+"pTjet1"   ,";p_{T}^{Jet1};EventsBIN"           ,100,0,600);
      hs.addHist(selection+"pTjet2"   ,";p_{T}^{Jet2};EventsBIN"           ,100,0,600);
      hs.addHist(selection+"etajet1"   ,";|#eta_{Jet1}|;EventsBIN"           ,100,0,2.5);
      hs.addHist(selection+"etajet2"   ,";|#eta_{Jet2}|;EventsBIN"           ,100,0,2.5);
      hs.addHist(selection+"etajet2"   ,";|#eta_{Jet2}|;EventsBIN"           ,100,0,2.5);
      hs.addHist(selection+"deepCSV"   ,";deepCSV;EventsBIN"           ,100,0,1.);
   }

   
   auto const dss = cfg.datasets.getDatasubset(vsDatasubsets[0]);
   TFile file(dss.getPath(),"read");
   if (file.IsZombie()) {
      return;
   }
   io::log * ("Processing '"+dss.datasetName+"' ");
;
   
   hs.setCurrentSample(dss.name);
   
   //Lumi weight for current sample
   float lumi_weight=dss.xsec/float(dss.Ngen)*cfg.lumi;

   TTreeReader reader(cfg.treeName, &file);
   TTreeReaderValue<float> w_pu(reader, "pu_weight");
   TTreeReaderValue<UInt_t> runNo(reader, "runNo");
   TTreeReaderValue<UInt_t> lumNo(reader, "lumNo");
   TTreeReaderValue<ULong64_t> evtNo(reader, "evtNo");
   TTreeReaderValue<Char_t> w_mc(reader, "mc_weight");
   TTreeReaderValue<std::vector<float>> w_pdf(reader, "pdf_weights");
   TTreeReaderValue<std::vector<tree::Muon>>     muons    (reader, "muons");
   TTreeReaderValue<std::vector<tree::Electron>> electrons(reader, "electrons");
   TTreeReaderValue<std::vector<tree::Jet>>      jets     (reader, "jets");
   TTreeReaderValue<std::vector<tree::GenParticle>> genParticles(reader, "genParticles");
   TTreeReaderValue<std::vector<tree::IntermediateGenParticle>> intermediateGenParticles(reader, "intermediateGenParticles");     
   TTreeReaderValue<std::vector<tree::Particle>> triggerObjects(reader, "hltEG165HE10Filter");
   TTreeReaderValue<tree::MET> MET(reader, "met");
   TTreeReaderValue<tree::MET> GENMET(reader, "met_gen");
   TTreeReaderValue<tree::MET> MET_JESu(reader, "met_JESu");
   TTreeReaderValue<tree::MET> MET_JESd(reader, "met_JESd");
   TTreeReaderValue<tree::MET> MET_Puppi(reader, "metPuppi");
   TTreeReaderValue<tree::MET> MET_NoHF(reader, "metNoHF");
   TTreeReaderValue<tree::MET> MET_Calo(reader, "metCalo");
   TTreeReaderValue<tree::MET> MET_Raw(reader, "met_raw");
   TTreeReaderValue<int> n_Interactions(reader, "true_nPV");
   TTreeReaderValue<float> HTgen(reader, "genHt");
   TTreeReaderValue<bool> is_ee   (reader, "ee");
   TTreeReaderValue<bool> is_emu   (reader, "emu");
   TTreeReaderValue<bool> is_mumu   (reader, "mumu");
   TTreeReaderValue<float> mll   (reader, "mll");
   TTreeReaderValue<float> mt2   (reader, "MT2");
   TTreeReaderValue<float> genMT2   (reader, "genMT2");
   TTreeReaderValue<float> genMT2neutrino   (reader, "genMT2neutrino");
   TTreeReaderValue<float> sf_lep1(reader, "lepton1SF");
   TTreeReaderValue<float> sf_lep2(reader, "lepton2SF");
   TTreeReaderValue<TLorentzVector> genTop(reader, "pseudoTop");
   TTreeReaderValue<TLorentzVector> genAntiTop(reader, "pseudoAntiTop");
   TTreeReaderValue<TLorentzVector> genLepton(reader, "pseudoLepton");
   TTreeReaderValue<TLorentzVector> genAntiLepton(reader, "pseudoAntiLepton");
   TTreeReaderValue<TLorentzVector> genTau(reader, "pseudoTau");
   TTreeReaderValue<TLorentzVector> genAntiTau(reader, "pseudoAntiTau");
   TTreeReaderValue<int> genLeptonPdgId(reader, "pseudoLeptonPdgId");
   TTreeReaderValue<int> genAntiLeptonPdgId(reader, "pseudoAntiLeptonPdgId");
   TTreeReaderValue<TLorentzVector> genB(reader, "pseudoBJet");
   TTreeReaderValue<TLorentzVector> genAntiB(reader, "pseudoAntiBJet");
   TTreeReaderValue<TLorentzVector> genNeutrino(reader, "pseudoNeutrino");
   TTreeReaderValue<TLorentzVector> genAntiNeutrino(reader, "pseudoAntiNeutrino");
   TTreeReaderValue<TLorentzVector> genWMinus(reader, "pseudoWMinus");
   TTreeReaderValue<TLorentzVector> genWPlus(reader, "pseudoWPlus");
   TTreeReaderValue<int> genDecayMode_pseudo(reader, "ttbarPseudoDecayMode");
   // ~TTreeReaderValue<TLorentzVector> genTop(reader, "genTop");    //Alternative genParticles, which are not based on RIVET
   // ~TTreeReaderValue<TLorentzVector> genAntiTop(reader, "genAntiTop");
   // ~TTreeReaderValue<TLorentzVector> genLepton(reader, "genLepton");
   // ~TTreeReaderValue<TLorentzVector> genAntiLepton(reader, "genAntiLepton");
   // ~TTreeReaderValue<TLorentzVector> genTau(reader, "genTau");
   // ~TTreeReaderValue<TLorentzVector> genAntiTau(reader, "genAntiTau");
   // ~TTreeReaderValue<int> genLeptonPdgId(reader, "genLeptonPdgId");
   // ~TTreeReaderValue<int> genAntiLeptonPdgId(reader, "genAntiLeptonPdgId");
   // ~TTreeReaderValue<TLorentzVector> genB(reader, "genB");
   // ~TTreeReaderValue<TLorentzVector> genAntiB(reader, "genAntiB");
   // ~TTreeReaderValue<TLorentzVector> genNeutrino(reader, "genNeutrino");
   // ~TTreeReaderValue<TLorentzVector> genAntiNeutrino(reader, "genAntiNeutrino");
   // ~TTreeReaderValue<TLorentzVector> genWMinus(reader, "genWMinus");
   // ~TTreeReaderValue<TLorentzVector> genWPlus(reader, "genWPlus");
   TTreeReaderValue<int> genDecayMode(reader, "ttbarDecayMode");

   int recoEvents=0;
   int iEv=0;
   int processEvents=cfg.processFraction*dss.entries;
   while (reader.Next()){
      iEv++;
      if (iEv>processEvents) break;
      if (iEv%(std::max(processEvents/10,1))==0){
         io::log*".";
         io::log.flush();
      }
      
      float fEventWeight=*w_pu * *w_mc * *sf_lep1 * *sf_lep2;     //Set event weight also taking lepton scale factors into account
      hs.setFillWeight(fEventWeight);
      
      float const met=MET->p.Pt();
      float const met_puppi=MET_Puppi->p.Pt();
      float const genMet=GENMET->p.Pt();
      
      bool rec_selection=true;
      bool pseudo_selection=true;
      
      //Baseline selection (separation into ee, emu, mumu already done at TreeWriter)
      std::vector<bool> channel={*is_ee,*is_mumu,*is_emu};
      TLorentzVector p_l1;
      TLorentzVector p_l2;
      int flavor_l1=0;  //1 for electron and 2 for muon
      int flavor_l2=0;
      bool muonLead=true; //Boolean for emu channel
      TString cat="";
      
      rec_selection=selection::diLeptonSelection(*electrons,*muons,channel,p_l1,p_l2,flavor_l1,flavor_l2,cat,muonLead);
            
      std::vector<tree::Jet> cjets;
      std::vector<tree::Jet> BJets;
      std::vector<bool> ttbarSelection=selection::ttbarSelection(p_l1,p_l2,met_puppi,channel,*jets,cjets,BJets);
      if(!std::all_of(ttbarSelection.begin(), ttbarSelection.end(), [](bool v) { return v; })) rec_selection=false;
      
      if (*genDecayMode_pseudo==0) pseudo_selection=false; //pseudo baseline selection
            
      if(rec_selection==false) continue;  //fill the following histograms only with events selected by the reco baseline selection
      
      recoEvents++;
          
      TString selection="Fakes";
      if (*genDecayMode>3) selection="Taus";
      else if (pseudo_selection) selection="NoFakes";
      
      hs.fill(selection+"/met",met);
      hs.fill(selection+"/met1000",met);
      hs.fill(selection+"/pTlep1",p_l1.Pt());
      hs.fill(selection+"/pTlep2",p_l2.Pt());
      hs.fill(selection+"/etalep1",abs(p_l1.Eta()));
      hs.fill(selection+"/etalep2",abs(p_l2.Eta()));
      hs.fill(selection+"/pTbJet",BJets[0].p.Pt());
      hs.fill(selection+"/etabJet",abs(BJets[0].p.Eta()));
      hs.fill(selection+"/pTjet1",cjets[0].p.Pt());
      hs.fill(selection+"/pTjet2",cjets[1].p.Pt());
      hs.fill(selection+"/etajet1",abs(cjets[0].p.Eta()));
      hs.fill(selection+"/etajet2",abs(cjets[1].p.Eta()));
      hs.fill(selection+"/deepCSV",BJets[0].bTagDeepCSV);
            
   }// evt loop
   io::log<<"";
   
   std::cout<<recoEvents<<std::endl;
   
   hs.scaleLumi();
   hs.mergeOverflow();
   file.Close();
   
   // ~std::vector<TString> samplesToCombine={"TTbar"};
   std::vector<TString> samplesToCombine={"TTbar_diLepton"};
   hs.combineFromSubsamples(samplesToCombine);
   
   // Save 1d histograms
   io::RootFileSaver saver_hist(TString::Format("histograms_%s.root",cfg.treeVersion.Data()),TString::Format("diff_fakes%.1f",cfg.processFraction*100),false);
   hs.saveHistograms(saver_hist,samplesToCombine);
   
}

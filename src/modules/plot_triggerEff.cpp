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
#include <iostream>
#include <fstream>
#include <iomanip>

Config const &cfg=Config::get();

// ~std::vector<float> const &Edges_ee_mumu={20,40,60,80,100,150,200};
// ~std::vector<float> const &Edges_emu={20,40,60,80,100,150,200};
std::vector<float> const &Edges_ee_mumu={20,50,90,200};
std::vector<float> const &Edges_emu={20,50,80,120,200};

// Function to plot 2D eff or SF
void plot2D(TH2 &hist, TString const &cat, TString const &savePath, io::RootFileSaver const &saver, float const &zMin=0.9, float const &zMax=1.0){
   TCanvas can;
   gPad->SetRightMargin(0.2);
   gPad->SetLeftMargin(0.13);
   hist.GetYaxis()->SetTitleOffset(1.0);
   hist.GetZaxis()->SetLabelOffset(0.02);
   hist.SetMaximum(zMax);
   hist.SetMinimum(zMin);
   hist.SetMarkerSize(1.2);
   hist.SetStats(0);
   gStyle->SetPaintTextFormat(".3f");
   hist.Draw("colz text e");
   TLatex label=gfx::cornerLabel(cat,1);
   label.Draw();
   saver.save(can,savePath,true,false,true);
}

// Function to derive unc. range in percent
std::pair<float,float> getRelUncRange(TH2 const &hist){
   float minUnc = 1.;
   float maxUnc = 0.;
   float currentUnc;
   for(int i=0; i<=hist.GetNbinsX();i++){
      for(int j=0; j<=hist.GetNbinsY();j++){
         if (hist.GetBinContent(i,j)>0) {
            currentUnc = hist.GetBinError(i,j)/hist.GetBinContent(i,j);
            minUnc = std::min(minUnc,currentUnc);
            maxUnc = std::max(maxUnc,currentUnc);
         }
      }
   }
   std::pair<float,float> result{minUnc*100,maxUnc*100};
   return result;
}

// Function to print line for LaTex table to file
TString printUncRange(TH2F const &histUp,TH2F const &histDown, TString const name){
   std::pair<float,float> uncRangeUp = getRelUncRange(histUp);
   std::pair<float,float> uncRangeDown = getRelUncRange(histDown);
   TString out = TString::Format(" & %s & $%.2f-%.2f$ & $%.2f-%.2f$\\\\\n",name.Data(),uncRangeDown.first,uncRangeDown.second,uncRangeUp.first,uncRangeUp.second);
   return out;
}
   
   

// Function to derive and plot eff. for data and MC in 1D
void eff1D(io::RootFileReader const &histReader, io::RootFileSaver const &saver, TString const &dataName, TString const &mcName){
   TCanvas can;
   for(TString selection:{"baselineTrigger","nJets2","nJets3+","nPV30-","nPF30+","MET150-","MET150+"}){
      // ~for(TString trigg:{"analysisTrigg","doubleTrigg_DZ","doubleTrigg","singleTrigg"}){
      for(TString trigg:{"analysisTrigg"}){
         for(TString channel:{"ee","mumu","emu"}){
            std::vector<TString> varVector = {"pTl1","pTl2","etal1","etal2"};
            if (channel=="emu") varVector = {"pTle","pTlmu","etale","etalmu"};
            for(TString var:varVector){
               // Read needed histograms
               TH1F* base_data=histReader.read<TH1F>(selection+"/baselineTrigger/"+channel+"/"+var+"/"+dataName);
               TH1F* data=histReader.read<TH1F>(selection+"/"+trigg+"/"+channel+"/"+var+"/"+dataName);
               TH1F* base_MC=histReader.read<TH1F>(selection+"/baselineTrigger/"+channel+"/"+var+"/"+mcName);
               TH1F* MC=histReader.read<TH1F>(selection+"/"+trigg+"/"+channel+"/"+var+"/"+mcName);
                              
               // Derive integrated efficiency with correct unc.
               float efficiency_data=data->Integral()/float(base_data->Integral());
               float e_u_data= TEfficiency::ClopperPearson(base_data->Integral(),data->Integral(),0.682689492137,true) -efficiency_data;
               float e_d_data=-TEfficiency::ClopperPearson(base_data->Integral(),data->Integral(),0.682689492137,false)+efficiency_data;
               float efficiency_MC=MC->Integral()/float(base_MC->Integral());
               float e_u_MC= TEfficiency::ClopperPearson(base_MC->Integral(),MC->Integral(),0.682689492137,true) -efficiency_MC;
               float e_d_MC=-TEfficiency::ClopperPearson(base_MC->Integral(),MC->Integral(),0.682689492137,false)+efficiency_MC;
               
               // Get binned efficiency
               TH1F axis=hist::getRatio(*MC,*base_MC,"Efficiency",hist::ONLY1);
               TEfficiency eff_data_hist(*data,*base_data);
               TEfficiency eff_MC_hist(*MC,*base_MC);
               
               //Set drawing options
               eff_MC_hist.SetLineColor(kRed);
               eff_MC_hist.SetMarkerColor(kRed);
               axis.SetMaximum(1.1);
               axis.SetMinimum(0.75);
               axis.SetStats(0);
               axis.GetYaxis()->SetTitleOffset(0.8);
               axis.Draw("axis");
               
               //Draw line at max eff.
               TLine * aline = new TLine();
               aline->SetLineWidth(2);
               aline->SetLineStyle(7);
               aline->DrawLine(0.,1.,200.,1.);
               
               // Draw eff. for MC and data
               eff_data_hist.Draw("pe1 same");
               eff_MC_hist.Draw("same");
               
               
               TString cat;   // set channel label
               if (channel.Contains("ee")) cat="ee";
               else if (channel.Contains("emu")) cat="e#mu";
               else if (channel.Contains("mumu")) cat="#mu#mu";
               // ~TLatex label=gfx::cornerLabel(channel+", "+trigg,3);
               TLatex label=gfx::cornerLabel(cat,1);
               label.Draw();
               
               gfx::LegendEntries legE;
               TString dataString = "Data";
               TString mcString = "Simulation";
               // ~legE.append(eff_data_hist,TString::Format(dataName+" (eff.=%.2f^{+%.2f}_{-%.2f}%%)",efficiency_data*100,e_u_data*100,e_d_data*100),"lep");
               // ~legE.append(eff_MC_hist,TString::Format(mcName+" (eff.=%.2f^{+%.2f}_{-%.2f}%%)",efficiency_MC*100,e_u_MC*100,e_d_MC*100),"lep");
               legE.append(eff_data_hist,TString::Format(dataString+" (#varepsilon=%.1f%%)",efficiency_data*100),"lep");
               legE.append(eff_MC_hist,TString::Format(mcString+" (#varepsilon=%.1f%%)",efficiency_MC*100),"lep");
               TLegend leg=legE.buildLegend(.52,.75,1-(gPad->GetRightMargin()+0.02),-1);
               leg.Draw();
               
               saver.save(can,selection+"/"+trigg+"/"+channel+"/"+var,true,false,true);
            }
         }
      }
   }
}

// Function to derive and plot eff. and SF for data and MC in 2D
void SF2D(io::RootFileReader const &histReader, io::RootFileSaver const &saver, io::RootFileSaver const &saverHist, TString const &dataName, TString const &mcName){
   TCanvas can;
   for(TString selection:{"baselineTrigger","nJets2","nJets3+","nPV30-","nPF30+","MET150-","MET150+"}){
      // ~for(TString trigg:{"analysisTrigg","doubleTrigg_DZ","doubleTrigg","singleTrigg"}){
      for(TString trigg:{"analysisTrigg"}){
         for(TString channel:{"ee","mumu","emu"}){
            TString var=(channel!="emu")? "pTl1_pTl2":"pTlmu_pTle";
            TH2F* base_data=histReader.read<TH2F>(selection+"/baselineTrigger/"+channel+"/"+var+"/"+dataName);
            TH2F* data=histReader.read<TH2F>(selection+"/"+trigg+"/"+channel+"/"+var+"/"+dataName);
            TH2F* base_MC=histReader.read<TH2F>(selection+"/baselineTrigger/"+channel+"/"+var+"/"+mcName);
            TH2F* MC=histReader.read<TH2F>(selection+"/"+trigg+"/"+channel+"/"+var+"/"+mcName);
            std::vector<float> Edges = (channel!="emu")? Edges_emu:Edges_ee_mumu;
            *base_data=hist::rebinned(*base_data,Edges,Edges);
            *data=hist::rebinned(*data,Edges,Edges);
            *base_MC=hist::rebinned(*base_MC,Edges,Edges);
            *MC=hist::rebinned(*MC,Edges,Edges);
            
            for(int i=0; i<=MC->GetNbinsX();i++){
               for(int j=0; j<=MC->GetNbinsY();j++){
                  if (MC->GetBinContent(i,j) > base_MC->GetBinContent(i,j)){
                     MC->SetBinContent(i,j,base_MC->GetBinContent(i,j));
                  }
               }
            }
                        
            TEfficiency eff_data(*data,*base_data);
            TH2* eff_data_hist_eUp = eff_data.CreateHistogram();
            TH2* eff_data_hist_eDown = eff_data.CreateHistogram();
            TEfficiency eff_MC(*MC,*base_MC);
            TH2* eff_MC_hist_eUp = eff_MC.CreateHistogram();
            TH2* eff_MC_hist_eDown = eff_MC.CreateHistogram();
            //Set correct error calculating (need since due to hist::rebinned TEfficiency thinks weights are used due to float precision)
            eff_data.SetUseWeightedEvents(false);
            eff_MC.SetUseWeightedEvents(false);
            eff_data.SetStatisticOption(TEfficiency::kFCP);
            eff_MC.SetStatisticOption(TEfficiency::kFCP);
            for(int i=0; i<=eff_MC_hist_eUp->GetNbinsX();i++){
               for(int j=0; j<=eff_MC_hist_eUp->GetNbinsY();j++){
                  eff_MC_hist_eUp->SetBinError(i,j,eff_MC.GetEfficiencyErrorUp(eff_MC.GetGlobalBin(i,j)));
                  eff_data_hist_eUp->SetBinError(i,j,eff_data.GetEfficiencyErrorUp(eff_data.GetGlobalBin(i,j)));
                  eff_MC_hist_eDown->SetBinError(i,j,eff_MC.GetEfficiencyErrorLow(eff_MC.GetGlobalBin(i,j)));
                  eff_data_hist_eDown->SetBinError(i,j,eff_data.GetEfficiencyErrorLow(eff_data.GetGlobalBin(i,j)));
               }
            }
            
            TString cat;
            if (channel.Contains("ee")) cat="ee";
            else if (channel.Contains("emu")) cat="e#mu";
            else if (channel.Contains("mumu")) cat="#mu#mu";
            TLatex label=gfx::cornerLabel(cat,1);
            gPad->SetRightMargin(0.2);
            gPad->SetLeftMargin(0.13);
            
            // Plot eff. with up and down stat error
            std::vector<TH2*> histVec = {eff_MC_hist_eUp,eff_data_hist_eUp,eff_MC_hist_eDown,eff_data_hist_eDown};
            std::vector<TString> nameVec = {"_MC_statUp","_data_statUp","_MC_statDown","_data_statDown"};
            int i = 0;
            for (auto tempHist : histVec){
               plot2D(*tempHist,cat+","+nameVec[i],selection+"/"+trigg+"/"+channel+"/"+var+nameVec[i],saver);
               i++;
            }
            // Derive SF
            eff_data_hist_eUp->Divide(eff_MC_hist_eUp);
            eff_data_hist_eDown->Divide(eff_MC_hist_eDown);
            
            histVec = {eff_data_hist_eUp,eff_data_hist_eDown};
            nameVec = {"_SF_statUp","_SF_statDown"};
            
            // Plot SF with up and down stat error
            i = 0;
            for (auto tempHist : histVec){
               can.Clear();
               plot2D(*tempHist,cat+","+nameVec[i],selection+"/"+trigg+"/"+channel+"/"+var+nameVec[i],saver);
               saverHist.save(*tempHist,selection+"/"+trigg+"/"+channel+"/"+var+nameVec[i]);
               i++;
            }
            
         }
      }
   }
}

// Function to derive correlation between dilepton and baseline triggers
void alpha(io::RootFileReader const &histReader, io::RootFileSaver const &saver, io::RootFileSaver const &saverHist, TString const &mcName){
   TCanvas can;
   // ~for(TString trigg:{"analysisTrigg","doubleTrigg_DZ","doubleTrigg","singleTrigg"}){
   for(TString trigg:{"analysisTrigg"}){
      for(TString channel:{"ee","mumu","emu"}){
         TString var=(channel!="emu")? "pTl1_pTl2":"pTlmu_pTle";
         TH2F* base_MC=histReader.read<TH2F>("noTrigger/baselineTrigger/"+channel+"/"+var+"/"+mcName);
         TH2F* all_MC=histReader.read<TH2F>("noTrigger/all/"+channel+"/"+var+"/"+mcName);
         TH2F* diLepton_MC=histReader.read<TH2F>("noTrigger/"+trigg+"/"+channel+"/"+var+"/"+mcName);
         TH2F* both_MC=histReader.read<TH2F>("baselineTrigger/"+trigg+"/"+channel+"/"+var+"/"+mcName);
         
         std::vector<float> Edges = (channel!="emu")? Edges_emu:Edges_ee_mumu;
         *all_MC=hist::rebinned(*all_MC,Edges,Edges);
         *base_MC=hist::rebinned(*base_MC,Edges,Edges);
         *diLepton_MC=hist::rebinned(*diLepton_MC,Edges,Edges);
         *both_MC=hist::rebinned(*both_MC,Edges,Edges);

         TEfficiency eff_base(*base_MC,*all_MC);
         TEfficiency eff_diLepton(*diLepton_MC,*all_MC);
         TEfficiency eff_both(*both_MC,*all_MC);
         TH2* eff_base_hist = eff_base.CreateHistogram();
         TH2* eff_diLepton_hist = eff_diLepton.CreateHistogram();
         TH2* eff_both_hist = eff_both.CreateHistogram();
         TH2* alpha = eff_diLepton.CreateHistogram();
         
         TString cat;
         if (channel.Contains("ee")) cat="ee";
         else if (channel.Contains("emu")) cat="e#mu";
         else if (channel.Contains("mumu")) cat="#mu#mu";
         
         
         gPad->SetRightMargin(0.2);
         gPad->SetLeftMargin(0.13);
         
         // Plot eff. used for correlation estimation
         std::vector<TH2*> histVec = {eff_base_hist,eff_diLepton_hist,eff_both_hist};
         std::vector<TString> nameVec = {"_eff_baseline","_eff_trigger","_eff_both"};
         int i = 0;
         for (auto tempHist : histVec){
            plot2D(*tempHist,cat+","+nameVec[i],"alpha/"+trigg+"/"+channel+"/"+var+nameVec[i],saver,0.,1.);
            i++;
         }
         
         TLatex label=gfx::cornerLabel(cat,1);
         alpha->Multiply(eff_base_hist);
         alpha->Divide(eff_both_hist);
         
         saverHist.save(*alpha,"alpha/"+trigg+"/"+channel+"/"+var+"_alpha");      //Save hist for further unc. calculation
         
         plot2D(*alpha,cat+",""alpha","alpha/"+trigg+"/"+channel+"/"+var+"_alpha",saver);
      }
   }
}

void lumiWeightedSF(io::RootFileReader const &histReader, io::RootFileSaver const &saver, io::RootFileSaver const &saverHist, TString const &datasetName, TString const &mcName){
   std::map<int,std::map<TString,float>> lumiFractionsPerEra = {
                                                            {0,{{"B",0.295},{"C",0.132},{"D",0.217},{"E",0.206},{"F",0.150}}},   //2016B,2016C,2016D,2016E,2016F preVFP
                                                            {1,{{"F",0.036},{"G",0.450},{"H",0.514}}},   //2016F,2016G,2016H postVFP
                                                            {2,{{"B",0.116},{"C",0.233},{"D",0.102},{"E",0.223},{"F",0.326}}},   //2017B,2017C,2017D,2017E,2017F
                                                            {3,{{"A",0.233},{"B",0.118},{"C",0.116},{"D",0.532}}}};        //2018A,2018B,2018C,2018D
   std::map<int,std::vector<TString>> eraNames = {
                                                            {0,{"B","C","D","E","F"}},   //2016B,2016C,2016D,2016E,2016F preVFP
                                                            {1,{"F","G","H"}},   //2016F,2016G,2016H postVFP
                                                            {2,{"B","C","D","E","F"}},   //2017B,2017C,2017D,2017E,2017F
                                                            {3,{"A","B","C","D"}}};        //2018A,2018B,2018C,2018D
   
   
   TCanvas can;
   for(TString selection:{"baselineTrigger"}){
      // ~for(TString trigg:{"analysisTrigg","doubleTrigg_DZ","doubleTrigg","singleTrigg"}){
      for(TString trigg:{"analysisTrigg"}){
         for(TString channel:{"ee","mumu","emu"}){
            TString var=(channel!="emu")? "pTl1_pTl2":"pTlmu_pTle";
            int i = 0;
            std::vector<float> Edges = (channel!="emu")? Edges_emu:Edges_ee_mumu;
            TH2F lumiWeightSF("","",Edges.size()-1,&Edges[0],Edges.size()-1,&Edges[0]);
            for (auto dsName: cfg.datasets.getDatasubsetNames({datasetName})){      
                      
               TH2F* base_data=histReader.read<TH2F>(selection+"/baselineTrigger/"+channel+"/"+var+"/"+dsName);
               TH2F* data=histReader.read<TH2F>(selection+"/"+trigg+"/"+channel+"/"+var+"/"+dsName);
               TH2F* base_MC=histReader.read<TH2F>(selection+"/baselineTrigger/"+channel+"/"+var+"/"+mcName);
               TH2F* MC=histReader.read<TH2F>(selection+"/"+trigg+"/"+channel+"/"+var+"/"+mcName);
               
               *base_data=hist::rebinned(*base_data,Edges,Edges);
               *data=hist::rebinned(*data,Edges,Edges);
               *base_MC=hist::rebinned(*base_MC,Edges,Edges);
               *MC=hist::rebinned(*MC,Edges,Edges);

               TEfficiency eff_data(*data,*base_data);
               TH2* eff_data_hist = eff_data.CreateHistogram();
               TEfficiency eff_MC(*MC,*base_MC);
               TH2* eff_MC_hist = eff_MC.CreateHistogram();
               //Set correct error calculating (need since due to hist::rebinned TEfficiency thinks weights are used due to float precision)
               eff_data.SetUseWeightedEvents(false);
               eff_MC.SetUseWeightedEvents(false);
               eff_data.SetStatisticOption(TEfficiency::kFCP);
               eff_MC.SetStatisticOption(TEfficiency::kFCP);
               
               for(int i=0; i<=eff_MC_hist->GetNbinsX();i++){
                  for(int j=0; j<=eff_MC_hist->GetNbinsY();j++){
                     eff_MC_hist->SetBinError(i,j,eff_MC.GetEfficiencyErrorUp(eff_MC.GetGlobalBin(i,j)));
                     eff_data_hist->SetBinError(i,j,eff_data.GetEfficiencyErrorUp(eff_data.GetGlobalBin(i,j)));
                  }
               }
               
               TString cat;
               if (channel.Contains("ee")) cat="ee";
               else if (channel.Contains("emu")) cat="e#mu";
               else if (channel.Contains("mumu")) cat="#mu#mu";
               
               //Plot eff
               plot2D(*eff_data_hist,cat+", Era"+eraNames[cfg.year_int][i],selection+"/"+trigg+"/"+channel+"/Era"+eraNames[cfg.year_int][i]+"/"+var+"_data",saver);

               
               // Derive SF
               eff_data_hist->Divide(eff_MC_hist);

               plot2D(*eff_data_hist,cat+", Era"+eraNames[cfg.year_int][i],selection+"/"+trigg+"/"+channel+"/Era"+eraNames[cfg.year_int][i]+"/"+var+"_SF",saver);
               
               // Store for lumi weighted sum
               
               eff_data_hist->Scale(lumiFractionsPerEra[cfg.year_int][eraNames[cfg.year_int][i]]);
               lumiWeightSF.Add(eff_data_hist);
               if((i+1)==eraNames[cfg.year_int].size()){  //store if last era is reached
                  saverHist.save(lumiWeightSF,selection+"/"+trigg+"/"+channel+"/"+var+"_SF_lumiWeighted");
                  lumiWeightSF.SetStats(0);
                  plot2D(lumiWeightSF,cat+", lumiWeighted",selection+"/"+trigg+"/"+channel+"/"+var+"_SF_lumiWeighted",saver);
               }
               i++;
            }
         }
      }
   }
}

void phaseSpaceUnc(io::RootFileReader const &sFReader, io::RootFileSaver const &saverHist){
      TCanvas can;
   // ~for(TString trigg:{"analysisTrigg","doubleTrigg_DZ","doubleTrigg","singleTrigg"}){
   for(TString trigg:{"analysisTrigg"}){
      for(TString channel:{"ee","mumu","emu"}){
         TString var=(channel!="emu")? "pTl1_pTl2":"pTlmu_pTle";
         std::vector<float> Edges = (channel!="emu")? Edges_emu:Edges_ee_mumu;
         TH2F maxDiff("","",Edges.size()-1,&Edges[0],Edges.size()-1,&Edges[0]);
         
         TH2F* nominalSF=sFReader.read<TH2F>("baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_statUp");
         
         for(TString selection:{"nJets2","nJets3+","nPV30-","nPF30+","MET150-","MET150+"}){     //loop over different phase spaces to derive largest difference
            TH2F* phaseSpaceSF=sFReader.read<TH2F>(selection+"/"+trigg+"/"+channel+"/"+var+"_SF_statUp");
            
            for(int i=0; i<=phaseSpaceSF->GetNbinsX();i++){
               for(int j=0; j<=phaseSpaceSF->GetNbinsY();j++){
                  float tempDiff = abs(nominalSF->GetBinContent(i,j)-phaseSpaceSF->GetBinContent(i,j));
                  if (tempDiff>maxDiff.GetBinError(i,j)) maxDiff.SetBinError(i,j,tempDiff);     //if difference larger as before, store in unc.
               }
            }
         }
         
         for(int i=0; i<=nominalSF->GetNbinsX();i++){       // Set nominal SF (unc. already derived before)
            for(int j=0; j<=nominalSF->GetNbinsY();j++){
               maxDiff.SetBinContent(i,j,nominalSF->GetBinContent(i,j));
            }
         }
            
         saverHist.save(maxDiff,"baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_systPhaseSpace");
      }
   }
}

void finalSFunc(io::RootFileReader const &sFReader, io::RootFileSaver const &saver, io::RootFileSaver const &saverHist, io::RootFileSaver const &saverSF){
   std::ofstream uncFile;
   uncFile.open(cfg.outputDirectory+"/triggerEff/triggerUnc.txt");
   // ~for(TString trigg:{"analysisTrigg","doubleTrigg_DZ","doubleTrigg","singleTrigg"}){
   for(TString trigg:{"analysisTrigg"}){
      for(TString channel:{"ee","mumu","emu"}){
         TString var=(channel!="emu")? "pTl1_pTl2":"pTlmu_pTle";
         TH2F* alpha=sFReader.read<TH2F>("alpha/"+trigg+"/"+channel+"/"+var+"_alpha");
         TH2F* nominalSF_alpha=sFReader.read<TH2F>("baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_statUp");
         TH2F* nominalSF_totalSyst=sFReader.read<TH2F>("baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_statUp");
         TH2F* nominalSF_lumiWeight=sFReader.read<TH2F>("baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_statUp");
         TH2F* lumiWeighted=sFReader.read<TH2F>("baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_lumiWeighted");
         TH2F* totalUp=sFReader.read<TH2F>("baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_statUp");
         TH2F* totalDown=sFReader.read<TH2F>("baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_statDown");
         TH2F* totalMean=sFReader.read<TH2F>("baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_statDown");
         TH2F* statUp=sFReader.read<TH2F>("baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_statUp");
         TH2F* statDown=sFReader.read<TH2F>("baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_statDown");
         
         bool isAnalysisTrigg = trigg=="analysisTrigg";
         
         TString cat;
         if (channel.Contains("ee")) cat="ee";
         else if (channel.Contains("emu")) cat="e#mu";
         else if (channel.Contains("mumu")) cat="#mu#mu";
         
         //Derive unc. based on alpha
         for(int i=0; i<=alpha->GetNbinsX();i++){
            for(int j=0; j<=alpha->GetNbinsY();j++){
               nominalSF_alpha->SetBinError(i,j,abs(alpha->GetBinContent(i,j)-1.));
            }
         }
         saverHist.save(*nominalSF_alpha,"baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_systAlpha");
         if(isAnalysisTrigg) saverSF.save(*nominalSF_alpha,channel+"_SF_systAlpha");
         
         //Derive unc. based on lumiWeightedSF
         for(int i=0; i<=lumiWeighted->GetNbinsX();i++){
            for(int j=0; j<=lumiWeighted->GetNbinsY();j++){
               nominalSF_lumiWeight->SetBinError(i,j,abs(lumiWeighted->GetBinContent(i,j)-nominalSF_lumiWeight->GetBinContent(i,j)));
            }
         }
         saverHist.save(*nominalSF_lumiWeight,"baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_systLumiWeight");
         if(isAnalysisTrigg) saverSF.save(*nominalSF_lumiWeight,channel+"_SF_systLumiWeight");
         
         //Save phaseSpace unc.
         TH2F* nominalSF_phaseSpace=sFReader.read<TH2F>("baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_systPhaseSpace");
         if(isAnalysisTrigg) saverSF.save(*nominalSF_phaseSpace,channel+"_SF_systPhaseSpace");
         plot2D(*nominalSF_phaseSpace,cat+",_SF_systPhaseSpace","baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_systPhaseSpace",saver);
         
         //Derive total syst unc.
         for(int i=0; i<=alpha->GetNbinsX();i++){
            for(int j=0; j<=alpha->GetNbinsY();j++){
               nominalSF_totalSyst->SetBinError(i,j,sqrt(pow(nominalSF_alpha->GetBinError(i,j),2)
                                                         +pow(nominalSF_lumiWeight->GetBinError(i,j),2)
                                                         +pow(nominalSF_phaseSpace->GetBinError(i,j),2)));
            }
         }
         saverHist.save(*nominalSF_totalSyst,"baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_systTotal");
         if(isAnalysisTrigg) saverSF.save(*nominalSF_totalSyst,channel+"_SF_systTotal");
         
         //Derive total unc.
         for(int i=0; i<=alpha->GetNbinsX();i++){
            for(int j=0; j<=alpha->GetNbinsY();j++){
               totalUp->SetBinError(i,j,sqrt(pow(nominalSF_totalSyst->GetBinError(i,j),2)+pow(totalUp->GetBinError(i,j),2)));
               totalDown->SetBinError(i,j,sqrt(pow(nominalSF_totalSyst->GetBinError(i,j),2)+pow(totalDown->GetBinError(i,j),2)));
               totalMean->SetBinError(i,j,(totalUp->GetBinError(i,j)+totalDown->GetBinError(i,j))/2.);
            }
         }
         
         saverHist.save(*totalUp,"baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_totalUp");
         saverHist.save(*totalDown,"baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_totalDown");
         saverHist.save(*totalMean,"baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_totalMean");
         if(isAnalysisTrigg){
             saverSF.save(*totalUp,channel+"_SF_totalUp");
             saverSF.save(*totalDown,channel+"_SF_totalDown");
         }
         plot2D(*totalUp,cat+",SF_totalUp","baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_totalUp",saver);
         plot2D(*totalDown,cat+",SF_totalDown","baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_totalDown",saver);
         plot2D(*totalMean,cat+"","baselineTrigger/"+trigg+"/"+channel+"/"+var+"_SF_totalMean",saver);
         
         //Store min max unc. in File
         if(isAnalysisTrigg){
            uncFile<<cat<<std::endl;
            uncFile<<printUncRange(*statUp,*statDown,"stat");
            uncFile<<printUncRange(*nominalSF_lumiWeight,*nominalSF_lumiWeight,"syst Era");
            uncFile<<printUncRange(*nominalSF_alpha,*nominalSF_alpha,"syst Alpha");
            uncFile<<printUncRange(*nominalSF_phaseSpace,*nominalSF_phaseSpace,"syst PhaseSpace");
            uncFile<<printUncRange(*nominalSF_totalSyst,*nominalSF_totalSyst,"syst Total");
            uncFile<<printUncRange(*totalUp,*totalDown,"Total");
            uncFile<<std::endl;            
         }
      }
   }
   uncFile.close();
}
         
         
         

extern "C"
void run()
{  
   
   std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
   std::cout<<"!MET and TTbar_diLepton have to be defined in config!"<<std::endl;
   std::cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
   
   Systematic::Systematic currentSystematic(cfg.systematic);
   
   TString inputLoc = TString::Format("triggerEff/%s/%s_merged_%s.root",currentSystematic.name().Data(),"histograms",cfg.treeVersion.Data());
   io::RootFileReader histReader(inputLoc,TString::Format("triggerEff%.1f",cfg.processFraction*100));
   io::RootFileSaver saver(TString::Format("triggerEff/plots_triggerEff%.1f.root",cfg.processFraction*100),"plot_triggerEff");
   io::RootFileSaver saverHist(TString::Format("triggerEff/hists_triggerEff%.1f.root",cfg.processFraction*100),"triggerEff");
   io::RootFileSaver saverSF(TString::Format("../data/TriggerSF_%s.root",cfg.year.Data()),"",false,false);

   
   //Plot 1D efficiencies
   eff1D(histReader,saver,"MET","TTbar_diLepton");
   
   //Plot 2D scale factors (Data/MC)
   SF2D(histReader,saver,saverHist,"MET","TTbar_diLepton");
   
   // Derive correlation between dilepton and reference trigger
   alpha(histReader,saver,saverHist,"TTbar_diLepton");
   
   // Derive SF per era and compare to nominal
   lumiWeightedSF(histReader,saver,saverHist,"MET","TTbar_diLepton");
   
   //Derive unc. based on phaseSpace (new saver needed, since sfReader uses same file)
   saverHist.closeFile();
   io::RootFileSaver saverHist2(TString::Format("triggerEff/hists_triggerEff%.1f.root",cfg.processFraction*100),"triggerEff");
   io::RootFileReader sfReader(TString::Format("triggerEff/hists_triggerEff%.1f.root",cfg.processFraction*100),"triggerEff");
   phaseSpaceUnc(sfReader,saverHist2);
   
   // Derive final uncertainties (new saver needed, since sfReader uses same file)
   saverHist2.closeFile();
   sfReader.closeFile();
   io::RootFileSaver saverHist3(TString::Format("triggerEff/hists_triggerEff%.1f.root",cfg.processFraction*100),"triggerEff");
   io::RootFileReader sfReader2(TString::Format("triggerEff/hists_triggerEff%.1f.root",cfg.processFraction*100),"triggerEff");
   finalSFunc(sfReader2,saver,saverHist3,saverSF);
   
}

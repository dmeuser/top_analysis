//Script to plot DNN resolution studies

#include "Config.hpp"
#include "tools/hist.hpp"
#include "tools/physics.hpp"
#include "tools/io.hpp"
#include "tools/weighters.hpp"

#include <TFile.h>
#include <TF1.h>
#include <TF2.h>
#include <TStyle.h>
#include <TCanvas.h>
#include <TColor.h>

std::pair<float,int> getChi2NDF(TH1F* hist_res, TH1F* hist_true) {
   if (hist_res->GetNbinsX()!=hist_true->GetNbinsX()){
      std::cout<<"Histograms in Chi2 function have different number of bins"<<std::endl;
      throw;
   }
   else {
      float chi2 = 0;
      for (int i=1; i<=hist_res->GetNbinsX(); i++) {
         float diff = hist_res->GetBinContent(i)-hist_true->GetBinContent(i);
         float err = hist_res->GetBinError(i)*1.;
         if(err==0) continue;
         chi2+= diff*diff/(err*err);
      }
      std::pair<float,int>result(chi2,hist_res->GetNbinsX());
      return result;
   }
}

TH1F getRMS(const TH2F* hist2D){
   TProfile::SetDefaultSumw2();
   TProfile TTbar_profile_RMS=*(hist2D->ProfileX("ProfileRMS",1,-1,"s"));
   // ~TH1F RMS("","",100,0,100);
   TH1F RMS("","",hist2D->GetNbinsX(),0,hist2D->GetXaxis()->GetXmax());
   for (int i=1; i<=TTbar_profile_RMS.GetNbinsX(); i++){
      RMS.SetBinContent(i,TTbar_profile_RMS.GetBinError(i));
      RMS.SetBinError(i,0.0001);
   }
   return RMS;
}

TH1F getAbsoluteValues(const TProfile* hist){
   TH1F hist_abs("","",hist->GetNbinsX(),0,hist->GetXaxis()->GetXmax());
   hist_abs.GetXaxis()->SetTitle(hist->GetXaxis()->GetTitle());
   for (int i=1; i<=hist_abs.GetNbinsX(); i++){
      // ~hist_abs.SetBinContent(i,abs(hist->GetBinContent(i)));
      hist_abs.SetBinContent(i,hist->GetBinContent(i));
      hist_abs.SetBinError(i,hist->GetBinError(i));
   }
   return hist_abs;
}

Config const &cfg=Config::get();

extern "C"
void run()
{
   io::RootFileSaver saver(TString::Format("plots%.1f.root",cfg.processFraction*100),"plot_DNNresolution");
   
   TCanvas can;
   TCanvas canCombined;
   gfx::LegendEntries leCombined;
   can.cd();
   
   //2016
   // ~TString inputFile=TString::Format("binningUnfolding/diLepton_amcatnlo_xyTarget_JetLepXY_50EP_2D_bothCorr_noOverflow_%.1f.root",cfg.processFraction*100);
   // ~TString inputFile=TString::Format("binningUnfolding/T2tt_650_350_Inlusive_amcatnlo_xyComponent_JetLepXY_50EP__diff_xy_2016_20210521-1448normDistr_%.1f.root",cfg.processFraction*100);
   // ~TString inputFile=TString::Format("binningUnfolding/T1tttt_1500_100_Inlusive_amcatnlo_xyComponent_JetLepXY_50EP__diff_xy_2016_20210521-1448normDistr_%.1f.root",cfg.processFraction*100);
   // ~TString inputFile=TString::Format("binningUnfolding/T2tt_650_350_Inlusive_amcatnlo__PuppiMET-genMET_2016_20210609-1056normDistr_%.1f.root",cfg.processFraction*100);
   
   // ~TString inputFile=TString::Format("binningUnfolding/diLepton_Inlusive_amcatnlo_xyComponent_JetLepXY_50EP_withoutMETinputs__diff_xy_2016_20210615-1210normDistr_%.1f.root",cfg.processFraction*100);
   // ~TString inputFile=TString::Format("binningUnfolding/T2tt_650_350_Inlusive_amcatnlo_xyComponent_JetLepXY_50EP_withoutMETinputs__diff_xy_2016_20210615-1210normDistr_%.1f.root",cfg.processFraction*100);
   
   //2018
   TString inputFile=TString::Format("binningUnfolding/diLepton_Inlusive_amcatnlo_xyComponent_JetLepXY_50EP__diff_xy_2018_20210519-1014normDistr_%.1f.root",cfg.processFraction*100);
   
   bool isBSM=inputFile.Contains("T2tt") || inputFile.Contains("T1tttt");
   
   io::RootFileReader histReader(inputFile);
   
   std::vector<float> axisLimits_low={0,0,0,50,50,50};
   std::vector<float> axisLimits_high={150,150,200,250,300,500};
   
   std::vector<TString> metRegions={"p_{T}^{miss}<40GeV","40GeV<p_{T}^{miss}<80GeV","80GeV<p_{T}^{miss}<120GeV","120GeV<p_{T}^{miss}<160GeV","160GeV<p_{T}^{miss}<230GeV","230GeV<p_{T}^{miss}",""};
   
   int i=0;
   // ~for(TString target :{"","_ptnunu"}){
   // ~for(TString target :{""}){
   for(TString target :{"","_phi"}){
      
      double axis_limit=500;
      double axis_limit_low=0;
      double axis_limit_diff=150;
      // ~double axis_limit_diff=100;
      int numBins_diff=100;
      int numBins=250;
      if(target=="_phi") {
         axis_limit=3.2;
         axis_limit_low=-3.2;
         axis_limit_diff=1.6;
         numBins_diff=200;
      }
      if(isBSM){
         numBins=20;
         numBins_diff=20;
         if(target=="_phi") numBins_diff=40;
      }
      
      TH1F genMET_histCombined("","",numBins,axis_limit_low,axis_limit);
      TH1F PuppiMET_histCombined("","",numBins,axis_limit_low,axis_limit);
      TH1F PuppiMETcorr_histCombined("","",numBins,axis_limit_low,axis_limit);
      TH1F PFMET_histCombined("","",numBins,axis_limit_low,axis_limit);
      
      TH1F PuppiMET_diffCombined("","",numBins_diff,-1.*axis_limit_diff,axis_limit_diff);
      TH1F PuppiMETcorr_diffCombined("","",numBins_diff,-1.*axis_limit_diff,axis_limit_diff);
      TH1F PFMET_diffCombined("","",numBins_diff,-1.*axis_limit_diff,axis_limit_diff);
      
      for (TString bin :{"bin1","bin2","bin3","bin4","bin5","bin6","combined"}){
         //Distribution
         can.cd();
         TH1F* genMET_hist;
         TH1F* PuppiMET_hist;
         TH1F* PuppiMETcorr_hist;
         TH1F* PFMET_hist;
         
         if(bin!="combined") {
            if(target=="" || target=="_phi") genMET_hist = histReader.read<TH1F>("binningUnfolding_DNN/DNN/GenMET"+target+"_"+bin);
            else genMET_hist = histReader.read<TH1F>("binningUnfolding_DNN/DNN/Ptnunu_"+bin);
            PuppiMET_hist = histReader.read<TH1F>("binningUnfolding_DNN/DNN/PuppiMET"+target+"_"+bin);
            PuppiMETcorr_hist = histReader.read<TH1F>("binningUnfolding_DNN/DNN/PuppiMETcorr"+target+"_"+bin);
            PFMET_hist = histReader.read<TH1F>("binningUnfolding_DNN/DNN/PFMET"+target+"_"+bin);
         }
         
         if(isBSM && bin!="combined"){
            genMET_hist->Rebin(25);
            PuppiMET_hist->Rebin(25);
            PuppiMETcorr_hist->Rebin(25);
            PFMET_hist->Rebin(25);
         }
         else{
            genMET_hist->Rebin(2);
            PuppiMET_hist->Rebin(2);
            PuppiMETcorr_hist->Rebin(2);
            PFMET_hist->Rebin(2);
         }
         
         hist::mergeOverflow(*genMET_hist,true);
         hist::mergeOverflow(*PuppiMET_hist,true);
         hist::mergeOverflow(*PuppiMETcorr_hist,true);
         hist::mergeOverflow(*PFMET_hist,true);
         
         if(bin!="combined") {
            genMET_histCombined.Add(genMET_hist);     //add for combined plot
            PuppiMET_histCombined.Add(PuppiMET_hist);
            PuppiMETcorr_histCombined.Add(PuppiMETcorr_hist);
            PFMET_histCombined.Add(PFMET_hist);
         }
         else{
            genMET_hist=&genMET_histCombined;
            PuppiMET_hist=&PuppiMET_histCombined;
            PuppiMETcorr_hist=&PuppiMETcorr_histCombined;
            PFMET_hist=&PFMET_histCombined;
         }
         
         genMET_hist->SetLineColor(kGreen);
         PuppiMET_hist->SetLineColor(kRed);
         PuppiMETcorr_hist->SetLineColor(kBlue);
         PFMET_hist->SetLineColor(kBlack);
         
         genMET_hist->SetStats(0);
         if(target=="")genMET_hist->GetXaxis()->SetTitle("MET (GeV)");
         else genMET_hist->GetXaxis()->SetTitle("#phi(MET)");
         genMET_hist->GetYaxis()->SetTitle("Events/Bin");
         genMET_hist->GetYaxis()->SetRangeUser(0,1.5*PuppiMET_hist->GetMaximum());
         if(target!="_phi") genMET_hist->GetXaxis()->SetRangeUser(axisLimits_low[i],axisLimits_high[i]);
         
         genMET_hist->Draw("hist");
         PuppiMET_hist->Draw("hist same");
         PuppiMETcorr_hist->Draw("hist same");
         PFMET_hist->Draw("hist same");
         
         
         auto chi2_Puppi=getChi2NDF(genMET_hist,PuppiMET_hist);
         auto chi2_DNN=getChi2NDF(genMET_hist,PuppiMETcorr_hist);
         auto chi2_PF=getChi2NDF(genMET_hist,PFMET_hist);
         
         gfx::LegendEntries le;
         le.append(*PuppiMET_hist,TString::Format("PuppiMET(#chi^{2}/ndf=%.0f/%i)",chi2_Puppi.first,chi2_Puppi.second),"l");
         le.append(*PFMET_hist,TString::Format("PFMET(#chi^{2}/ndf=%.0f/%i)",chi2_PF.first,chi2_PF.second),"l");
         le.append(*PuppiMETcorr_hist,TString::Format("DNN(#chi^{2}/ndf=%.0f/%i)",chi2_DNN.first,chi2_DNN.second),"l");
         if(target=="" || target=="_phi") le.append(*genMET_hist,"genMET","l");
         else le.append(*genMET_hist,"pTnunu","l");
         TLegend leg=le.buildLegend(.4,.7,1-1.5*gPad->GetRightMargin(),-1,1);
         leg.Draw();
         
         TLatex label_BSM=gfx::cornerLabel("T2tt_650_350",3);
         if(inputFile.Contains("T2tt")) label_BSM.Draw();
         
         TLatex label=gfx::cornerLabel(metRegions[i],1);
         label.Draw();
         saver.save(can,inputFile+"/dist"+target+"_"+bin,true,true);
         
         //Resolution
         TH1F* diff_hist;
         TH1F* diffPF_hist;
         TH1F* diffCorr_hist;
         
         if(bin!="combined") {
            diff_hist = histReader.read<TH1F>("binningUnfolding_DNN/DNN/diff"+target+"_"+bin);
            diffPF_hist = histReader.read<TH1F>("binningUnfolding_DNN/DNN/diffPF"+target+"_"+bin);
            diffCorr_hist = histReader.read<TH1F>("binningUnfolding_DNN/DNN/diffcorr"+target+"_"+bin);
         }
         
         if(isBSM && bin!="combined"){
            diff_hist->Rebin(10);
            diffPF_hist->Rebin(10);
            diffCorr_hist->Rebin(10);
         }
         else{
            diff_hist->Rebin(2);
            diffPF_hist->Rebin(2);
            diffCorr_hist->Rebin(2);
         }
         
         if(bin!="combined") {
            PuppiMET_diffCombined.Add(diff_hist);     //add for combined plot
            PuppiMETcorr_diffCombined.Add(diffPF_hist);
            PFMET_diffCombined.Add(diffCorr_hist);
         }
         else{
            diff_hist=&PuppiMET_diffCombined;
            diffPF_hist=&PuppiMETcorr_diffCombined;
            diffCorr_hist=&PFMET_diffCombined;
         }
         
         diff_hist->SetLineColor(kRed);
         diffPF_hist->SetLineColor(kBlack);
         diffCorr_hist->SetLineColor(kBlue);
         
         diff_hist->SetStats(0);
         diff_hist->GetYaxis()->SetRangeUser(0,1.4*diffCorr_hist->GetMaximum());
         if(target=="")diff_hist->GetXaxis()->SetTitle("genMET-recoMET (GeV)");
         else if (target=="_phi") diff_hist->GetXaxis()->SetTitle("#Delta#phi(genMET,recoMET)");
         diff_hist->GetYaxis()->SetTitle("Events/Bin");
         
         diff_hist->Draw("hist");
         diffPF_hist->Draw("hist same");
         diffCorr_hist->Draw("hist same");
         
         gfx::LegendEntries le2;
         le2.append(*diff_hist,TString::Format("PuppiMET(#mu=%.2f #sigma=%.2f)",diff_hist->GetMean(),diff_hist->GetRMS()),"l");
         le2.append(*diffPF_hist,TString::Format("PFMET(#mu=%.2f #sigma=%.2f)",diffPF_hist->GetMean(),diffPF_hist->GetRMS()),"l");
         le2.append(*diffCorr_hist,TString::Format("DNN(#mu=%.2f #sigma=%.2f)",diffCorr_hist->GetMean(),diffCorr_hist->GetRMS()),"l");
         TLegend leg2=le2.buildLegend(.45,.75,1-1.5*gPad->GetRightMargin(),-1,1);
         leg2.Draw();
         
         if(inputFile.Contains("T2tt")) label_BSM.Draw();
         
         TLatex label2=gfx::cornerLabel(metRegions[i],1);
         label2.Draw();
         saver.save(can,inputFile+"/diff"+target+"_"+bin,true,true);
         
         //DNN Output
         if(target==""){
            if(bin!="combined") {
               canCombined.cd();
               gPad->SetLogy();
               TH1F* DNN_output = histReader.read<TH1F>("binningUnfolding_DNN/DNN/DNNoutput_"+bin);
               
               hist::mergeOverflow(*DNN_output,true);
               
               DNN_output->Scale(1./DNN_output->Integral());
               
               DNN_output->SetLineColor(Color::next());
               DNN_output->SetStats(0);
               DNN_output->GetYaxis()->SetRangeUser(0.001,0.3);
               DNN_output->GetXaxis()->SetTitle("DNN Output");
               DNN_output->GetYaxis()->SetTitle("normalized distributions");
               
               if (i==0) DNN_output->Draw("hist");
               else DNN_output->Draw("same hist");
               
               leCombined.append(*DNN_output,bin,"l");
            }
         }
         i++;
      }
      i=0;
   }
   
   canCombined.cd();
   TLegend leg=leCombined.buildLegend(.45,.75,1-1.5*gPad->GetRightMargin(),-1,2);
   leg.Draw();
   saver.save(canCombined,inputFile+"/DNN_Output",true,true);
   
   //Profile Plots
   if(cfg.year_int==3){
      for(TString xAxis: {"","_vsPuppi","_vsDNN"}){
         can.cd();
         TH2F* Puppi_2D=histReader.read<TH2F>("binningUnfolding_DNN/DNN/diffPuppi_2D"+xAxis);
         TH2F* PF_2D=histReader.read<TH2F>("binningUnfolding_DNN/DNN/diffPF_2D"+xAxis);
         TH2F* DNN_2D=histReader.read<TH2F>("binningUnfolding_DNN/DNN/diffcorr_2D"+xAxis);
         
         Puppi_2D->RebinX(10);
         PF_2D->RebinX(10);
         DNN_2D->RebinX(10);
         
         TH1F Puppi_mean=getAbsoluteValues(Puppi_2D->ProfileX());
         TH1F Puppi_rms=getRMS(Puppi_2D);
         TH1F PF_mean=getAbsoluteValues(PF_2D->ProfileX());
         TH1F PF_rms=getRMS(PF_2D);
         TH1F DNN_mean=getAbsoluteValues(DNN_2D->ProfileX());
         TH1F DNN_rms=getRMS(DNN_2D);
         
         Puppi_mean.SetLineColor(kRed);
         Puppi_rms.SetLineColor(kRed);
         DNN_mean.SetLineColor(kBlue);
         DNN_rms.SetLineColor(kBlue);
         PF_mean.SetLineColor(kBlack);
         PF_rms.SetLineColor(kBlack);
         
         Puppi_rms.SetLineStyle(2);
         DNN_rms.SetLineStyle(2);
         PF_rms.SetLineStyle(2);
         
         Puppi_mean.GetXaxis()->SetTitle("genMET (GeV)");
         if (xAxis.Contains("DNN")) Puppi_mean.GetXaxis()->SetTitle("DNN MET (GeV)");
         else if (xAxis.Contains("Puppi")) Puppi_mean.GetXaxis()->SetTitle("Puppi MET (GeV)");         
         Puppi_mean.GetYaxis()->SetTitle("genMET-recoMET (GeV)");
         Puppi_mean.GetXaxis()->SetRangeUser(0,400);
         Puppi_mean.GetYaxis()->SetRangeUser(-75,75);
         Puppi_mean.SetStats(0);
         
         Puppi_mean.Draw("hist");
         Puppi_rms.Draw("hist same");
         PF_mean.Draw("hist same");
         PF_rms.Draw("hist same");
         DNN_mean.Draw("hist same");
         DNN_rms.Draw("hist same");
         
         gfx::LegendEntries le;
         le.append(Puppi_mean,"PuppiMET mean","l");
         le.append(Puppi_rms,"PuppiMET rms","l");
         le.append(PF_mean,"PFMET mean","l");
         le.append(PF_rms,"PFMET rms","l");
         le.append(DNN_mean,"DNN mean","l");
         le.append(DNN_rms,"DNN rms","l");
         TLegend leg2=le.buildLegend(.4,.7,1-1.5*gPad->GetRightMargin(),-1,2);
         leg2.Draw();
         TLine * aline = new TLine();
         aline->DrawLine(0,0,400,0);
         
         saver.save(can,inputFile+"/Profile_Comparison"+xAxis,true,true);
      }
   }
   
   //Profile Plots (emu only)
   if(cfg.year_int==3){
      can.cd();
      io::RootFileReader histReader1("/net/data_cms1b/user/dmeuser/top_analysis/2018/v03/minTrees/100.0/temp/MET.root","",false);
      io::RootFileReader histReader2("/net/data_cms1b/user/dmeuser/top_analysis/2018/v03/minTrees/100.0/temp/DNN.root","",false);
      io::RootFileReader histReader3("/net/data_cms1b/user/dmeuser/top_analysis/2018/v03/minTrees/100.0/temp/Puppi.root","",false);
      
      TH2F* Puppi_2D=histReader3.read<TH2F>("htemp");
      TH2F* PF_2D=histReader1.read<TH2F>("htemp");
      TH2F* DNN_2D=histReader2.read<TH2F>("htemp");
      
      Puppi_2D->RebinX(10);
      PF_2D->RebinX(10);
      DNN_2D->RebinX(10);
      
      TH1F Puppi_mean=getAbsoluteValues(Puppi_2D->ProfileX());
      TH1F Puppi_rms=getRMS(Puppi_2D);
      TH1F PF_mean=getAbsoluteValues(PF_2D->ProfileX());
      TH1F PF_rms=getRMS(PF_2D);
      TH1F DNN_mean=getAbsoluteValues(DNN_2D->ProfileX());
      TH1F DNN_rms=getRMS(DNN_2D);
      
      Puppi_mean.SetLineColor(kRed);
      Puppi_rms.SetLineColor(kRed);
      DNN_mean.SetLineColor(kBlue);
      DNN_rms.SetLineColor(kBlue);
      PF_mean.SetLineColor(kBlack);
      PF_rms.SetLineColor(kBlack);
      
      Puppi_rms.SetLineStyle(2);
      DNN_rms.SetLineStyle(2);
      PF_rms.SetLineStyle(2);
      
      Puppi_mean.GetXaxis()->SetTitle("genMET (GeV)");     
      Puppi_mean.GetYaxis()->SetTitle("genMET-recoMET (GeV)");
      Puppi_mean.GetXaxis()->SetRangeUser(0,400);
      Puppi_mean.GetYaxis()->SetRangeUser(-75,75);
      Puppi_mean.SetStats(0);
      
      Puppi_mean.Draw("hist");
      Puppi_rms.Draw("hist same");
      PF_mean.Draw("hist same");
      PF_rms.Draw("hist same");
      DNN_mean.Draw("hist same");
      DNN_rms.Draw("hist same");
      
      gfx::LegendEntries le;
      le.append(Puppi_mean,"PuppiMET mean","l");
      le.append(Puppi_rms,"PuppiMET rms","l");
      le.append(PF_mean,"PFMET mean","l");
      le.append(PF_rms,"PFMET rms","l");
      le.append(DNN_mean,"DNN mean","l");
      le.append(DNN_rms,"DNN rms","l");
      TLegend leg2=le.buildLegend(.4,.7,1-1.5*gPad->GetRightMargin(),-1,2);
      leg2.Draw();
      TLine * aline = new TLine();
      aline->DrawLine(0,0,400,0);
      
      TLatex label=gfx::cornerLabel("emu",1);
      label.Draw();
      
      saver.save(can,inputFile+"/emu/Profile_Comparison",true,true);
   }
   
}

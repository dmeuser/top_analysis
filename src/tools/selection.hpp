#ifndef SELECTION_HPP__
#define SELECTION_HPP__

#include "tree/TreeParticles.hpp"
#include "physics.hpp"

namespace selection
{
   bool triggerSelection(std::vector<bool> const &diElectronTriggers, std::vector<bool> const &diMuonTriggers, std::vector<bool> const &electronMuonTriggers,
                        std::vector<bool> const &channel, bool const &isData, std::vector<bool> const &PD={}, bool const &is2016H=false);
   
   bool diLeptonSelection(std::vector<tree::Electron> const &electrons, std::vector<tree::Muon> const &muons, std::vector<bool> const &channel,
                        TLorentzVector &p_l1, TLorentzVector &p_l2, int &flavor_l1, int &flavor_l2, TString &cat, bool &muonLead);
   
   std::vector<bool> ttbarSelection(TLorentzVector const &p_l1, TLorentzVector const &p_l2, float const &met, std::vector<bool> const &channel,
                                    std::vector<tree::Jet> const &jets, std::vector<tree::Jet> &cleanJets, std::vector<tree::Jet> &bJets);
   std::vector<bool> ttbarSelection_looseJetID(TLorentzVector const &p_l1, TLorentzVector const &p_l2, float const &met, std::vector<bool> const &channel,
                                    std::vector<tree::Jet> const &jets, std::vector<tree::Jet> &cleanJets, std::vector<tree::Jet> &bJets);
} // namespace selection

#endif /* SELECTION_HPP__ */

#include "frtree.h"
#include "rooutil/rooutil.h"

int closureEvtType();

// ./process INPUTFILEPATH OUTPUTFILEPATH [NEVENTS]
int main(int argc, char** argv)
{
    // Argument checking
    if (argc < 3)
    {
        std::cout << "Usage:" << std::endl;
        std::cout << "  $ ./process INPUTFILES OUTPUTFILE [NEVENTS]" << std::endl;
        std::cout << std::endl;
        std::cout << "  INPUTFILES      comma separated file list" << std::endl;
        std::cout << "  OUTPUTFILE      output file name" << std::endl;
        std::cout << "  [LEPVERSION]    0: SS 1: 3L" << std::endl;
        std::cout << "  [NEVENTS=-1]    # of events to run over" << std::endl;
        std::cout << std::endl;
        return 1;
    }

    // Creating output file where we will put the outputs of the processing
    TFile* ofile = new TFile(argv[2], "recreate");

    // Create a TChain of the input files
    // The input files can be comma separated (e.g. "file1.root,file2.root") or with wildcard (n.b. be sure to escape)
    TChain* ch = RooUtil::FileUtil::createTChain("t", argv[1]);

    // Version of lepton to run
    int lepversion = argc > 3 ? atoi(argv[3]) : 0;

    // Number of events to loop over
    int nEvents = argc > 4 ? atoi(argv[4]) : -1;

    // Create a Looper object to loop over input files
    RooUtil::Looper<frtree> looper(ch, &fr, nEvents);

    // Hist map for retreiving histograms with fake rates and pileup reweights
    RooUtil::HistMap purewgt("histmap/puw_2017.root:puw_central");
    RooUtil::HistMap qcd_mu("histmap/fakerate.root:Mu_ptcorretarolledcoarse_qcd_fakerate");
    RooUtil::HistMap qcd_el("histmap/fakerate.root:El_ptcorretarolledcoarse_qcd_fakerate");

    // Variables
    float weight;
    bool presel;
    const float muiso_thresh = lepversion == 0 ? 0.03 : 0.07;
    const float eliso_thresh = lepversion == 0 ? 0.03 : 0.05;
    float jet_pt0;
    float MT;
    int muidx;
    int elidx;
    float muptcorr;
    float elptcorr;
    float ptcorr;
    bool onemu_cuts;
    bool oneel_cuts;
    bool onemuloose_cuts;
    bool oneelloose_cuts;
    // The pt corr v. eta are used to parametrize the fake rates
    // The boundaries are stored in std::vector
    const std::vector<float> eta_bounds = {0.0, 1.6, 2.4};
    const std::vector<float> etafine_bounds = {0.0, 0.8, 1.6, 2.4};
    const std::vector<float> ptcorr_bounds = {0., 20., 25., 30., 35., 50., 150.};
    const std::vector<float> ptcorrcoarse_bounds = {0., 20., 25., 30., 35., 150.};

    // Cutflow utility object that creates a tree structure of cuts
    RooUtil::Cutflow cutflow(ofile);

    cutflow.addCut("Presel"                                     , [&]() { return presel                                                                                                         ; } , [&]() { return weight                                             ; } ) ; 
    cutflow.getCut("Presel"); 
    cutflow.addCutToLastActiveCut("TwoMuHLT8"                   , [&]() { return (fr.nVlep()==2) * (fr.mc_HLT_SingleIsoMu8()  > 0) * (fr.MllSS() > 60.) * (fr.MllSS() < 120.)                   ; } , [&]() { return fr.mc_HLT_SingleIsoMu8()                           ; } ) ; 
    cutflow.getCut("Presel"); 
    cutflow.addCutToLastActiveCut("TwoMuHLT17"                  , [&]() { return (fr.nVlep()==2) * (fr.mc_HLT_SingleIsoMu17() > 0) * (fr.MllSS() > 60.) * (fr.MllSS() < 120.)                   ; } , [&]() { return fr.mc_HLT_SingleIsoMu17()                          ; } ) ; 
    cutflow.getCut("Presel"); 
    cutflow.addCutToLastActiveCut("TwoElHLT8"                   , [&]() { return (fr.nVlep()==2) * (fr.mc_HLT_SingleIsoEl8()  > 0) * (fr.MllSS() > 60.) * (fr.MllSS() < 120.) * (jet_pt0 > 40.) ; } , [&]() { return fr.mc_HLT_SingleIsoEl8()                           ; } ) ; 
    cutflow.getCut("Presel"); 
    cutflow.addCutToLastActiveCut("TwoElHLT23"                  , [&]() { return (fr.nVlep()==2) * (fr.mc_HLT_SingleIsoEl23() > 0) * (fr.MllSS() > 60.) * (fr.MllSS() < 120.) * (jet_pt0 > 40.) ; } , [&]() { return fr.mc_HLT_SingleIsoEl23()                          ; } ) ; 

    cutflow.getCut("Presel"); 
    cutflow.addCutToLastActiveCut("OneMu"                       , [&]() { return onemu_cuts                                                                                                     ; } , [&]() { return fr.mc_HLT_SingleIsoMu17()                          ; } ) ; 
    cutflow.addCutToLastActiveCut("OneMuHighMET"                , [&]() { return (fr.met_pt() > 30.)                                                                                            ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.addCutToLastActiveCut("OneMuEWKCR"                  , [&]() { return (MT > 80.) * (MT < 120.)                                                                                       ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneMu"); 
    cutflow.addCutToLastActiveCut("OneMuTightMR"                , [&]() { return (fr.met_pt() < 20.) * (MT < 20.)                                                                               ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("Presel"); 
    cutflow.addCutToLastActiveCut("OneEl"                       , [&]() { return oneel_cuts                                                                                                     ; } , [&]() { return fr.mc_HLT_SingleIsoEl23()                          ; } ) ; 
    cutflow.addCutToLastActiveCut("OneElHighMET"                , [&]() { return (fr.met_pt() > 30.)                                                                                            ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.addCutToLastActiveCut("OneElEWKCR"                  , [&]() { return (MT > 80.) * (MT < 120.)                                                                                       ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneEl"); 
    cutflow.addCutToLastActiveCut("OneElTightMR"                , [&]() { return (fr.met_pt() < 20.) * (MT < 20.)                                                                               ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("Presel"); 
    cutflow.addCutToLastActiveCut("OneMuLoose"                  , [&]() { return onemuloose_cuts                                                                                                ; } , [&]() { return fr.mc_HLT_SingleIsoMu17()                          ; } ) ; 
    cutflow.addCutToLastActiveCut("OneMuMR"                     , [&]() { return (fr.met_pt() < 20.) * (MT < 20.)                                                                               ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("Presel"); 
    cutflow.addCutToLastActiveCut("OneElLoose"                  , [&]() { return oneelloose_cuts                                                                                                ; } , [&]() { return fr.mc_HLT_SingleIsoEl23()                          ; } ) ; 
    cutflow.addCutToLastActiveCut("OneElMR"                     , [&]() { return (fr.met_pt() < 20.) * (MT < 20.)                                                                               ; } , [&]() { return 1.                                                 ; } ) ; 

    cutflow.getCut("OneMuEWKCR"); 
    cutflow.addCutToLastActiveCut("OneMuEWKCREta0Pt1"           , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[0])) == 2                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneMuEWKCR"); 
    cutflow.addCutToLastActiveCut("OneMuEWKCREta0Pt2"           , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[0])) == 3                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneMuEWKCR"); 
    cutflow.addCutToLastActiveCut("OneMuEWKCREta0Pt3"           , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[0])) == 4                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneMuEWKCR"); 
    cutflow.addCutToLastActiveCut("OneMuEWKCREta1Pt1"           , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[0])) == 7                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneMuEWKCR"); 
    cutflow.addCutToLastActiveCut("OneMuEWKCREta1Pt2"           , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[0])) == 8                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneMuEWKCR"); 
    cutflow.addCutToLastActiveCut("OneMuEWKCREta1Pt3"           , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[0])) == 9                ; } , [&]() { return 1.                                                 ; } ) ; 

    cutflow.getCut("OneMuMR"); 
    cutflow.addCutToLastActiveCut("OneMuMREta0Pt1"              , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[0])) == 2                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneMuMR"); 
    cutflow.addCutToLastActiveCut("OneMuMREta0Pt2"              , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[0])) == 3                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneMuMR"); 
    cutflow.addCutToLastActiveCut("OneMuMREta0Pt3"              , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[0])) == 4                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneMuMR"); 
    cutflow.addCutToLastActiveCut("OneMuMREta1Pt1"              , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[0])) == 7                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneMuMR"); 
    cutflow.addCutToLastActiveCut("OneMuMREta1Pt2"              , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[0])) == 8                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneMuMR"); 
    cutflow.addCutToLastActiveCut("OneMuMREta1Pt3"              , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[0])) == 9                ; } , [&]() { return 1.                                                 ; } ) ; 

    cutflow.getCut("OneElMR"); 
    cutflow.addCutToLastActiveCut("OneElMREta0Pt1"              , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, elptcorr, fabs(fr.lep_eta()[0])) == 2                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneElMR"); 
    cutflow.addCutToLastActiveCut("OneElMREta0Pt2"              , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, elptcorr, fabs(fr.lep_eta()[0])) == 3                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneElMR"); 
    cutflow.addCutToLastActiveCut("OneElMREta0Pt3"              , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, elptcorr, fabs(fr.lep_eta()[0])) == 4                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneElMR"); 
    cutflow.addCutToLastActiveCut("OneElMREta1Pt1"              , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, elptcorr, fabs(fr.lep_eta()[0])) == 7                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneElMR"); 
    cutflow.addCutToLastActiveCut("OneElMREta1Pt2"              , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, elptcorr, fabs(fr.lep_eta()[0])) == 8                ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("OneElMR"); 
    cutflow.addCutToLastActiveCut("OneElMREta1Pt3"              , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, elptcorr, fabs(fr.lep_eta()[0])) == 9                ; } , [&]() { return 1.                                                 ; } ) ; 

    cutflow.getCut("Presel"); 
    cutflow.addCutToLastActiveCut("MuClosure"                   , [&]() { return (closureEvtType() == 0) * (fr.nj() >= 2) * (fr.nVlep() == 2) * (fr.lep_pt()[0] > 25.) * (fr.lep_pt()[1] > 25.) ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("MuClosure"); 
    cutflow.addCutToLastActiveCut("MuClosureLoose"              , [&]() { return fr.lep_pass_VVV_cutbased_fo()[muidx] == 1                                                                      ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.addCutToLastActiveCut("MuClosureTight"              , [&]() { return fr.lep_pass_VVV_cutbased_tight()[muidx] == 1                                                                   ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("MuClosureTight"); 
    cutflow.addCutToLastActiveCut("MuClosureTightBVeto"         , [&]() { return fr.nb() == 0                                                                                                   ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("MuClosureTight"); 
    cutflow.addCutToLastActiveCut("MuClosureTightNbgeq2"        , [&]() { return fr.nb() >= 2                                                                                                   ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("MuClosureTight"); 
    cutflow.addCutToLastActiveCut("MuClosureTightNbgeq1"        , [&]() { return fr.nb() >= 1                                                                                                   ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("MuClosureLoose"); 
    cutflow.addCutToLastActiveCut("MuClosureTightPredict"       , [&]() { return 1                                                                                                              ; } , [&]() { return qcd_mu.eval(muptcorr , fabs(fr.lep_eta()[muidx]))  ; } ) ; 
    cutflow.getCut("MuClosureTightPredict"); 
    cutflow.addCutToLastActiveCut("MuClosureTightBVetoPredict"  , [&]() { return fr.nb() == 0                                                                                                   ; } , [&]() { return 1                                                  ; } ) ; 
    cutflow.getCut("MuClosureTightPredict"); 
    cutflow.addCutToLastActiveCut("MuClosureTightNbgeq2Predict" , [&]() { return fr.nb() >= 2                                                                                                   ; } , [&]() { return 1                                                  ; } ) ; 
    cutflow.getCut("MuClosureTightPredict"); 
    cutflow.addCutToLastActiveCut("MuClosureTightNbgeq1Predict" , [&]() { return fr.nb() >= 1                                                                                                   ; } , [&]() { return 1                                                  ; } ) ; 

    cutflow.getCut("MuClosureLoose"); 
    cutflow.addCutToLastActiveCut("MuClosureLooseEta0Pt1"       , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[muidx])) == 2            ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("MuClosureLoose"); 
    cutflow.addCutToLastActiveCut("MuClosureLooseEta0Pt2"       , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[muidx])) == 3            ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("MuClosureLoose"); 
    cutflow.addCutToLastActiveCut("MuClosureLooseEta0Pt3"       , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[muidx])) == 4            ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("MuClosureLoose"); 
    cutflow.addCutToLastActiveCut("MuClosureLooseEta1Pt1"       , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[muidx])) == 7            ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("MuClosureLoose"); 
    cutflow.addCutToLastActiveCut("MuClosureLooseEta1Pt2"       , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[muidx])) == 8            ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("MuClosureLoose"); 
    cutflow.addCutToLastActiveCut("MuClosureLooseEta1Pt3"       , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[muidx])) == 9            ; } , [&]() { return 1.                                                 ; } ) ; 

    cutflow.getCut("Presel"); 
    cutflow.addCutToLastActiveCut("ElClosure"                   , [&]() { return (closureEvtType() == 1) * (fr.nj() >= 2) * (fr.nVlep() == 2) * (fr.lep_pt()[0] > 25.) * (fr.lep_pt()[1] > 25.) ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("ElClosure"); 
    cutflow.addCutToLastActiveCut("ElClosureLoose"              , [&]() { return fr.lep_pass_VVV_cutbased_fo()[elidx] == 1                                                                      ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.addCutToLastActiveCut("ElClosureTight"              , [&]() { return fr.lep_pass_VVV_cutbased_tight()[elidx] == 1                                                                   ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("ElClosureTight"); 
    cutflow.addCutToLastActiveCut("ElClosureTightBVeto"         , [&]() { return fr.nb() == 0                                                                                                   ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("ElClosureTight"); 
    cutflow.addCutToLastActiveCut("ElClosureTightNbgeq2"        , [&]() { return fr.nb() >= 2                                                                                                   ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("ElClosureTight"); 
    cutflow.addCutToLastActiveCut("ElClosureTightNbgeq1"        , [&]() { return fr.nb() >= 1                                                                                                   ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("ElClosureLoose"); 
    cutflow.addCutToLastActiveCut("ElClosureTightPredict"       , [&]() { return 1                                                                                                              ; } , [&]() { return qcd_el.eval(elptcorr, fabs(fr.lep_eta()[elidx]))   ; } ) ; 
    cutflow.getCut("ElClosureTightPredict"); 
    cutflow.addCutToLastActiveCut("ElClosureTightBVetoPredict"  , [&]() { return fr.nb() == 0                                                                                                   ; } , [&]() { return 1                                                  ; } ) ; 
    cutflow.getCut("ElClosureTightPredict"); 
    cutflow.addCutToLastActiveCut("ElClosureTightNbgeq2Predict" , [&]() { return fr.nb() >= 2                                                                                                   ; } , [&]() { return 1                                                  ; } ) ; 
    cutflow.getCut("ElClosureTightPredict"); 
    cutflow.addCutToLastActiveCut("ElClosureTightNbgeq1Predict" , [&]() { return fr.nb() >= 1                                                                                                   ; } , [&]() { return 1                                                  ; } ) ; 

    cutflow.getCut("ElClosureLoose"); 
    cutflow.addCutToLastActiveCut("ElClosureLooseEta0Pt1"       , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, elptcorr, fabs(fr.lep_eta()[elidx])) == 2            ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("ElClosureLoose"); 
    cutflow.addCutToLastActiveCut("ElClosureLooseEta0Pt2"       , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, elptcorr, fabs(fr.lep_eta()[elidx])) == 3            ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("ElClosureLoose"); 
    cutflow.addCutToLastActiveCut("ElClosureLooseEta0Pt3"       , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, elptcorr, fabs(fr.lep_eta()[elidx])) == 4            ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("ElClosureLoose"); 
    cutflow.addCutToLastActiveCut("ElClosureLooseEta1Pt1"       , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, elptcorr, fabs(fr.lep_eta()[elidx])) == 7            ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("ElClosureLoose"); 
    cutflow.addCutToLastActiveCut("ElClosureLooseEta1Pt2"       , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, elptcorr, fabs(fr.lep_eta()[elidx])) == 8            ; } , [&]() { return 1.                                                 ; } ) ; 
    cutflow.getCut("ElClosureLoose"); 
    cutflow.addCutToLastActiveCut("ElClosureLooseEta1Pt3"       , [&]() { return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, elptcorr, fabs(fr.lep_eta()[elidx])) == 9            ; } , [&]() { return 1.                                                 ; } ) ; 

    // Print cut structure
    cutflow.printCuts();

    // Histogram utility object that is used to define the histograms
    RooUtil::Histograms histograms;
    histograms.addHistogram("Mll"                , 180 , 60 , 120      , [&]() { return fr.MllSS()                               ;} );
    histograms.addHistogram("MT"                 , 180 , 0  , 180      , [&]() { return MT                                       ;} );
    histograms.addHistogram("MET"                , 180 , 0  , 250      , [&]() { return fr.met_pt()                              ;} );
    histograms.addHistogram("Nvtx"               , 80  , 0  , 80       , [&]() { return fr.nVert()                               ;} );
    histograms.addHistogram("eta"                , 180 , -3 , 3        , [&]() { return fr.lep_eta()[0]                          ;} );
    histograms.addHistogram("pt"                 , 180 , 0  , 250      , [&]() { return fr.lep_pt()[0]                           ;} );
    histograms.addHistogram("ptcorr"             , 180 , 0  , 250      , [&]() { return ptcorr                                   ;} );
    histograms.addHistogram("nj"                 , 5   , 0  , 5        , [&]() { return fr.nj()                                  ;} );
    histograms.addHistogram("nVlep"              , 4   , 0  , 4        , [&]() { return fr.nVlep()                               ;} );
    histograms.addHistogram("nLlep"              , 4   , 0  , 4        , [&]() { return fr.nLlep()                               ;} );
    histograms.addHistogram("nTlep"              , 4   , 0  , 4        , [&]() { return fr.nTlep()                               ;} );
    histograms.addHistogram("iso"                , 180 , 0  , 0.4      , [&]() { return fr.lep_relIso03EAv2Lep()[0]              ;} );
    histograms.addHistogram("muiso"              , 180 , 0  , 0.4      , [&]() { return fr.lep_relIso03EAv2Lep()[muidx]          ;} );
    histograms.addHistogram("eliso"              , 180 , 0  , 0.4      , [&]() { return fr.lep_relIso03EAv2Lep()[elidx]          ;} );
    histograms.addHistogram("lepmotherid"        ,   7 , -4 , 3        , [&]() { return fr.lep_motherIdSS()[0]                   ;} );
    histograms.addHistogram("mumotherid"         ,   7 , -4 , 3        , [&]() { return fr.lep_motherIdSS()[muidx]               ;} );
    histograms.addHistogram("elmotherid"         ,   7 , -4 , 3        , [&]() { return fr.lep_motherIdSS()[elidx]               ;} );
    histograms.addHistogram("etavarbin"          , eta_bounds          , [&]() { return min((double)fabs(fr.lep_eta()[0]),2.3999);} );
    histograms.addHistogram("ptcorrvarbin"       , ptcorr_bounds       , [&]() { return min((double)ptcorr,149.99)               ;} );
    histograms.addHistogram("ptcorrvarbincoarse" , ptcorrcoarse_bounds , [&]() { return min((double)ptcorr,149.99)               ;} );

    histograms.addHistogram("ptcorretarolled"          , (eta_bounds.size()-1)     * (ptcorr_bounds.size()-1)        , 0  , (eta_bounds.size()-1)     * (ptcorr_bounds.size()-1)       , [&](){ return RooUtil::Calc::calcBin2D(ptcorr_bounds , eta_bounds , ptcorr , fabs(fr.lep_eta()[0]))             ;} );
    histograms.addHistogram("ptcorretarolledcoarse"    , (eta_bounds.size()-1)     * (ptcorrcoarse_bounds.size()-1)  , 0  , (eta_bounds.size()-1)     * (ptcorrcoarse_bounds.size()-1) , [&](){ return  RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, ptcorr, fabs(fr.lep_eta()[0]))         ;} );
    histograms.addHistogram("muptcorretarolledcoarse"  , (eta_bounds.size()-1)     * (ptcorrcoarse_bounds.size()-1)  , 0  , (eta_bounds.size()-1)     * (ptcorrcoarse_bounds.size()-1) , [&](){ return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, muptcorr, fabs(fr.lep_eta()[muidx]))    ;} );
    histograms.addHistogram("elptcorretarolledcoarse"  , (eta_bounds.size()-1)     * (ptcorrcoarse_bounds.size()-1)  , 0  , (eta_bounds.size()-1)     * (ptcorrcoarse_bounds.size()-1) , [&](){ return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, eta_bounds, elptcorr, fabs(fr.lep_eta()[elidx]))    ;} );
    histograms.addHistogram("muptcorretarolledfineeta" , (etafine_bounds.size()-1) * (ptcorrcoarse_bounds.size()-1)  , 0  , (etafine_bounds.size()-1) * (ptcorrcoarse_bounds.size()-1) , [&](){ return RooUtil::Calc::calcBin2D(ptcorrcoarse_bounds, etafine_bounds, muptcorr, fabs(fr.lep_eta()[muidx]));} );

    // Book cutflows
    cutflow.bookCutflows();

    // Book Histograms
    cutflow.bookHistograms(histograms); // if just want to book everywhere


    // Looping input file
    while (looper.nextEvent())
    {
        // Preliminary calculations
        //float weight = fr.isData() ? 1 : fr.evt_scale1fb() * 41.3 * purewgt.eval(fr.nTrueInt());
        weight = fr.isData() ? 1 : fr.evt_scale1fb() * 41.3;
        presel = fr.firstgoodvertex() == 0;
        presel &= fr.Flag_AllEventFilters() > 0;
        presel &= fr.evt_passgoodrunlist() > 0;

        jet_pt0 = fr.jets_p4().size() > 0 ? fr.jets_p4()[0].pt() : -999;
        MT = (TMath::Sqrt(2*fr.met_pt()*fr.lep_pt()[0]*(1.0-TMath::Cos(fr.lep_phi()[0]-fr.met_phi()))));
        muidx = abs(fr.lep_pdgId()[0]) == 13 ? 0 : 1;
        elidx = abs(fr.lep_pdgId()[0]) == 11 ? 0 : 1;
        muptcorr = fr.lep_pt()[muidx]*(1 + max((double) 0. , (double) fr.lep_relIso03EAv2Lep()[muidx]-muiso_thresh));
        elptcorr = fr.lep_pt()[elidx]*(1 + max((double) 0. , (double) fr.lep_relIso03EAv2Lep()[elidx]-eliso_thresh));
        ptcorr = abs(fr.lep_pdgId()[0]) == 13 ? muptcorr : elptcorr;
        onemu_cuts      = (fr.nVlep() == 1) * (fr.lep_pt()[0] > 25.) * (fr.lep_pass_VVV_cutbased_tight()[0] == 1) * (abs(fr.lep_pdgId()[0])==13) * (fr.mc_HLT_SingleIsoMu17() > 0) * (jet_pt0>40.);
        oneel_cuts      = (fr.nVlep() == 1) * (fr.lep_pt()[0] > 25.) * (fr.lep_pass_VVV_cutbased_tight()[0] == 1) * (abs(fr.lep_pdgId()[0])==11) * (fr.mc_HLT_SingleIsoEl23() > 0) * (jet_pt0>40.);
        onemuloose_cuts = (fr.nVlep() == 1) * (fr.lep_pt()[0] > 25.) * (fr.lep_pass_VVV_cutbased_fo()[0] == 1) * (abs(fr.lep_pdgId()[0])==13) * (fr.mc_HLT_SingleIsoMu17() > 0) * (jet_pt0>40.);
        oneelloose_cuts = (fr.nVlep() == 1) * (fr.lep_pt()[0] > 25.) * (fr.lep_pass_VVV_cutbased_fo()[0] == 1) * (abs(fr.lep_pdgId()[0])==11) * (fr.mc_HLT_SingleIsoEl23() > 0) * (jet_pt0>40.);

        cutflow.fill();
    }

    // Writing output file
    cutflow.saveOutput();

    // The below can be sometimes crucial
    delete ofile;
}

int closureEvtType()
{
    const std::vector<int>& genPart_pdgId = fr.genPart_pdgId();
    const std::vector<int>& genPart_motherId = fr.genPart_motherId();
    const std::vector<int>& lep_pdgId = fr.lep_pdgId();
    const std::vector<int>& lep_pass_VVV_cutbased_tight = fr.lep_pass_VVV_cutbased_tight();
    int evt_type = -1;
    int ngenlepW = 0;
//    DEBUGclass("genPart_pdgId->size() == %d", genPart_pdgId->size());
    for (unsigned int igen = 0; igen < genPart_pdgId.size(); ++igen)
    {
//        DEBUGclass("genPart_pdgId = %d, genPart_motherId = %d", genPart_pdgId->at(igen), genPart_motherId->at(igen));
        if (abs(genPart_pdgId.at(igen)) == 11 && abs(genPart_motherId.at(igen)) == 24)
        {
            evt_type = 0;
            ngenlepW++;
        }
        if (abs(genPart_pdgId.at(igen)) == 13 && abs(genPart_motherId.at(igen)) == 24)
        {
            evt_type = 1;
            ngenlepW++;
        }
        if (abs(genPart_pdgId.at(igen)) == 15 && abs(genPart_motherId.at(igen)) == 24)
        {
            evt_type = -1;
            ngenlepW++;
        }
    }
    if (ngenlepW == 0)
        evt_type = -2;
    else if (ngenlepW > 1)
        evt_type = -3;
    else if (ngenlepW != 1)
        evt_type = -4;

    if (
            (lep_pdgId.size() == 2) and
            (abs(lep_pdgId[0]*lep_pdgId[1]) == 143) and
            ((abs(lep_pdgId[0]) == 11 and lep_pass_VVV_cutbased_tight[0] == 1) or (abs(lep_pdgId[1]) == 11 and lep_pass_VVV_cutbased_tight[1] == 1)) and
            (evt_type == 0)
       )
        return 0;
    else if (
            (lep_pdgId.size() == 2) and
            (abs(lep_pdgId[0]*lep_pdgId[1]) == 143) and
            ((abs(lep_pdgId[0]) == 13 and lep_pass_VVV_cutbased_tight[0] == 1) or (abs(lep_pdgId[1]) == 13 and lep_pass_VVV_cutbased_tight[1] == 1)) and
            (evt_type == 1)
       )
        return 1;
    else
        return -1;
}
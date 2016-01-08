//////////////////////////////////////////////////////////
// This class has been automatically generated on
// Tue Aug 11 17:22:19 2015 by ROOT version 5.34/30
// from TTree anatree/analysis tree
// found on file: photana_hist.root
//////////////////////////////////////////////////////////

#ifndef looper_h
#define looper_h

#include <TROOT.h>
#include <TChain.h>
#include <TFile.h>

// Header file for the classes stored in the TTree if any.

// Fixed size dimensions of array or collections stored in the TTree if any.

class looper {
public :
   TTree          *fChain;   //!pointer to the analyzed TTree or TChain
   Int_t           fCurrent; //!current Tree number in a TChain

   // Declaration of leaf types
   Int_t           run;
   Int_t           subrun;
   Int_t           event;
   Double_t        evttime;
   Double_t        efield[3];
   Int_t           t0;
   Int_t           ntracks_reco;
   Int_t           ntrkhits[1];   //[ntracks_reco]
   Int_t           trkid[1];   //[ntracks_reco]
   Double_t        trkstartx[1];   //[ntracks_reco]
   Double_t        trkstarty[1];   //[ntracks_reco]
   Double_t        trkstartz[1];   //[ntracks_reco]
   Double_t        trkendx[1];   //[ntracks_reco]
   Double_t        trkendy[1];   //[ntracks_reco]
   Double_t        trkendz[1];   //[ntracks_reco]
   Double_t        trkstartdcosx[1];   //[ntracks_reco]
   Double_t        trkstartdcosy[1];   //[ntracks_reco]
   Double_t        trkstartdcosz[1];   //[ntracks_reco]
   Double_t        trkenddcosx[1];   //[ntracks_reco]
   Double_t        trkenddcosy[1];   //[ntracks_reco]
   Double_t        trkenddcosz[1];   //[ntracks_reco]
   Double_t        trkx[1][1000];   //[ntracks_reco]
   Double_t        trky[1][1000];   //[ntracks_reco]
   Double_t        trkz[1][1000];   //[ntracks_reco]
   Double_t        trktheta_xz[1];   //[ntracks_reco]
   Double_t        trktheta_yz[1];   //[ntracks_reco]
   Double_t        trketa_xy[1];   //[ntracks_reco]
   Double_t        trketa_zy[1];   //[ntracks_reco]
   Double_t        trktheta[1];   //[ntracks_reco]
   Double_t        trkphi[1];   //[ntracks_reco]
   Double_t        trkd2[1];   //[ntracks_reco]
   Double_t        trkdedx2[1][3][1000];   //[ntracks_reco]
   Double_t        trkdqdx[1][3][1000];   //[ntracks_reco]
   Double_t        trkpitch[1][3];   //[ntracks_reco]
   Double_t        trkpitchHit[1][3][1000];   //[ntracks_reco]
   Double_t        trkkinE[1][3];   //[ntracks_reco]
   Double_t        trkrange[1][3];   //[ntracks_reco]
   Double_t        trkTPC[1][3][1000];   //[ntracks_reco]
   Double_t        trkplaneid[1][3][1000];   //[ntracks_reco]
   Double_t        trkresrg[1][3][1000];   //[ntracks_reco]
   Double_t        trkPosx[1][3][1000];   //[ntracks_reco]
   Double_t        trkPosy[1][3][1000];   //[ntracks_reco]
   Double_t        trkPosz[1][3][1000];   //[ntracks_reco]
   Double_t        trklen[1];   //[ntracks_reco]
   Double_t        trklen_L[1];   //[ntracks_reco]
   Double_t        trkdQdxSum[1];   //[ntracks_reco]
   Double_t        trkdQdxAverage[1];   //[ntracks_reco]
   Double_t        trkdEdxSum[1];   //[ntracks_reco]
   Double_t        trkdEdxAverage[1];   //[ntracks_reco]
   Double_t        trkMCTruthT0[1];   //[ntracks_reco]
   Int_t           trkMCTruthTrackID[1];   //[ntracks_reco]
   Int_t           nMCParticles;
   Int_t           trkid_MC[34];   //[nMCParticles]
   Int_t           trkpdg_MC[34];   //[nMCParticles]
   Int_t           nTPCHits_MC[34];   //[nMCParticles]
   Int_t           StartInTPC_MC[34];   //[nMCParticles]
   Int_t           EndInTPC_MC[34];   //[nMCParticles]
   Int_t           trkMother_MC[34];   //[nMCParticles]
   Int_t           trkNumDaughters_MC[34];   //[nMCParticles]
   Int_t           trkFirstDaughter_MC[34];   //[nMCParticles]
   Int_t           trkPrimary_MC[34];   //[nMCParticles]
   Double_t        StartTime_MC[34];   //[nMCParticles]
   Double_t        trkstartx_MC[34];   //[nMCParticles]
   Double_t        trkstarty_MC[34];   //[nMCParticles]
   Double_t        trkstartz_MC[34];   //[nMCParticles]
   Double_t        trkendx_MC[34];   //[nMCParticles]
   Double_t        trkendy_MC[34];   //[nMCParticles]
   Double_t        trkendz_MC[34];   //[nMCParticles]
   Double_t        trkenergy_MC[34];   //[nMCParticles]
   Double_t        EnergyDeposited_MC[34];   //[nMCParticles]
   Double_t        trkmom_MC[34];   //[nMCParticles]
   Double_t        trkmom_XMC[34];   //[nMCParticles]
   Double_t        trkmom_YMC[34];   //[nMCParticles]
   Double_t        trkmom_ZMC[34];   //[nMCParticles]
   Double_t        trkstartdoc_XMC[34];   //[nMCParticles]
   Double_t        trkstartdoc_YMC[34];   //[nMCParticles]
   Double_t        trkstartdoc_ZMC[34];   //[nMCParticles]
   Double_t        mcpos_x[34];   //[nMCParticles]
   Double_t        mcpos_y[34];   //[nMCParticles]
   Double_t        mcpos_z[34];   //[nMCParticles]
   Double_t        mcang_x[34];   //[nMCParticles]
   Double_t        mcang_y[34];   //[nMCParticles]
   Double_t        mcang_z[34];   //[nMCParticles]
   Double_t        trktheta_xz_MC[34];   //[nMCParticles]
   Double_t        trktheta_yz_MC[34];   //[nMCParticles]
   Double_t        trktheta_MC[34];   //[nMCParticles]
   Double_t        trkphi_MC[34];   //[nMCParticles]
   Double_t        trketa_xy_MC[34];   //[nMCParticles]
   Double_t        trketa_zy_MC[34];   //[nMCParticles]
   Double_t        trkTPCLen_MC[34];   //[nMCParticles]
   Int_t           nhits;
   Int_t           nhits2;
   Int_t           nclust;
   Int_t           hit_plane[1237];   //[nhits2]
   Int_t           hit_tpc[1237];   //[nhits2]
   Int_t           hit_wire[1237];   //[nhits2]
   Int_t           hit_channel[1237];   //[nhits2]
   Double_t        hit_peakT[1237];   //[nhits2]
   Double_t        hit_charge[1237];   //[nhits2]
   Double_t        hit_ph[1237];   //[nhits2]
   Int_t           hit_trkid[1237];   //[nhits2]
   Int_t           flash_total;
   Double_t        flash_time[3];   //[flash_total]
   Double_t        flash_width[3];   //[flash_total]
   Double_t        flash_abstime[3];   //[flash_total]
   Double_t        flash_YCentre[3];   //[flash_total]
   Double_t        flash_YWidth[3];   //[flash_total]
   Double_t        flash_ZCentre[3];   //[flash_total]
   Double_t        flash_ZWidth[3];   //[flash_total]
   Double_t        flash_TotalPE[3];   //[flash_total]
   Int_t           ntrigs;
   Int_t           trig_time[3];   //[ntrigs]
   Int_t           trig_id[3];   //[ntrigs]

   // List of branches
   TBranch        *b_run;   //!
   TBranch        *b_subrun;   //!
   TBranch        *b_event;   //!
   TBranch        *b_evttime;   //!
   TBranch        *b_efield;   //!
   TBranch        *b_t0;   //!
   TBranch        *b_ntracks_reco;   //!
   TBranch        *b_ntrkhits;   //!
   TBranch        *b_trkid;   //!
   TBranch        *b_trkstartx;   //!
   TBranch        *b_trkstarty;   //!
   TBranch        *b_trkstartz;   //!
   TBranch        *b_trkendx;   //!
   TBranch        *b_trkendy;   //!
   TBranch        *b_trkendz;   //!
   TBranch        *b_trkstartdcosx;   //!
   TBranch        *b_trkstartdcosy;   //!
   TBranch        *b_trkstartdcosz;   //!
   TBranch        *b_trkenddcosx;   //!
   TBranch        *b_trkenddcosy;   //!
   TBranch        *b_trkenddcosz;   //!
   TBranch        *b_trkx;   //!
   TBranch        *b_trky;   //!
   TBranch        *b_trkz;   //!
   TBranch        *b_trktheta_xz;   //!
   TBranch        *b_trktheta_yz;   //!
   TBranch        *b_trketa_xy;   //!
   TBranch        *b_trketa_zy;   //!
   TBranch        *b_trktheta;   //!
   TBranch        *b_trkphi;   //!
   TBranch        *b_trkd2;   //!
   TBranch        *b_trkdedx2;   //!
   TBranch        *b_trkdqdx;   //!
   TBranch        *b_trkpitch;   //!
   TBranch        *b_trkpitchHit;   //!
   TBranch        *b_trkkinE;   //!
   TBranch        *b_trkrange;   //!
   TBranch        *b_trkTPC;   //!
   TBranch        *b_trkplaneid;   //!
   TBranch        *b_trkresrg;   //!
   TBranch        *b_trkPosx;   //!
   TBranch        *b_trkPosy;   //!
   TBranch        *b_trkPosz;   //!
   TBranch        *b_trklen;   //!
   TBranch        *b_trklen_L;   //!
   TBranch        *b_trkdQdxSum;   //!
   TBranch        *b_trkdQdxAverage;   //!
   TBranch        *b_trkdEdxSum;   //!
   TBranch        *b_trkdEdxAverage;   //!
   TBranch        *b_trkMCTruthT0;   //!
   TBranch        *b_trkMCTruthTrackID;   //!
   TBranch        *b_nMCParticles;   //!
   TBranch        *b_trkid_MC;   //!
   TBranch        *b_trkpdg_MC;   //!
   TBranch        *b_nTPCHits_MC;   //!
   TBranch        *b_StartInTPC_MC;   //!
   TBranch        *b_EndInTPC_MC;   //!
   TBranch        *b_trkMother_MC;   //!
   TBranch        *b_trkNumDaughters_MC;   //!
   TBranch        *b_trkFirstDaughter_MC;   //!
   TBranch        *b_trkPrimary_MC;   //!
   TBranch        *b_StartTime_MC;   //!
   TBranch        *b_trkstartx_MC;   //!
   TBranch        *b_trkstarty_MC;   //!
   TBranch        *b_trkstartz_MC;   //!
   TBranch        *b_trkendx_MC;   //!
   TBranch        *b_trkendy_MC;   //!
   TBranch        *b_trkendz_MC;   //!
   TBranch        *b_trkenergy_MC;   //!
   TBranch        *b_EnergyDeposited_MC;   //!
   TBranch        *b_trkmom_MC;   //!
   TBranch        *b_trkmom_XMC;   //!
   TBranch        *b_trkmom_YMC;   //!
   TBranch        *b_trkmom_ZMC;   //!
   TBranch        *b_trkstartdoc_XMC;   //!
   TBranch        *b_trkstartdoc_YMC;   //!
   TBranch        *b_trkstartdoc_ZMC;   //!
   TBranch        *b_mcpos_x;   //!
   TBranch        *b_mcpos_y;   //!
   TBranch        *b_mcpos_z;   //!
   TBranch        *b_mcang_x;   //!
   TBranch        *b_mcang_y;   //!
   TBranch        *b_mcang_z;   //!
   TBranch        *b_trktheta_xz_MC;   //!
   TBranch        *b_trktheta_yz_MC;   //!
   TBranch        *b_trktheta_MC;   //!
   TBranch        *b_trkphi_MC;   //!
   TBranch        *b_trketa_xy_MC;   //!
   TBranch        *b_trketa_zy_MC;   //!
   TBranch        *b_trkTPCLen_MC;   //!
   TBranch        *b_nhits;   //!
   TBranch        *b_nhits2;   //!
   TBranch        *b_nclust;   //!
   TBranch        *b_hit_plane;   //!
   TBranch        *b_hit_tpc;   //!
   TBranch        *b_hit_wire;   //!
   TBranch        *b_hit_channel;   //!
   TBranch        *b_hit_peakT;   //!
   TBranch        *b_hit_charge;   //!
   TBranch        *b_hit_ph;   //!
   TBranch        *b_hit_trkid;   //!
   TBranch        *b_flash_total;   //!
   TBranch        *b_flash_time;   //!
   TBranch        *b_flash_width;   //!
   TBranch        *b_flash_abstime;   //!
   TBranch        *b_flash_YCentre;   //!
   TBranch        *b_flash_YWidth;   //!
   TBranch        *b_flash_ZCentre;   //!
   TBranch        *b_flash_ZWidth;   //!
   TBranch        *b_flash_TotalPE;   //!
   TBranch        *b_ntrigs;   //!
   TBranch        *b_trig_time;   //!
   TBranch        *b_trig_id;   //!

   looper(TTree *tree=0);
   virtual ~looper();
   virtual Int_t    Cut(Long64_t entry);
   virtual Int_t    GetEntry(Long64_t entry);
   virtual Long64_t LoadTree(Long64_t entry);
   virtual void     Init(TTree *tree);
   virtual void     Loop();
   virtual Bool_t   Notify();
   virtual void     Show(Long64_t entry = -1);
};

#endif

#ifdef looper_cxx
looper::looper(TTree *tree) : fChain(0) 
{
// if parameter tree is not specified (or zero), connect the file
// used to generate this class and read the Tree.
   if (tree == 0) {
      TFile *f = (TFile*)gROOT->GetListOfFiles()->FindObject("photana_hist.root");
      if (!f || !f->IsOpen()) {
         f = new TFile("photana_hist.root");
      }
      TDirectory * dir = (TDirectory*)f->Get("photana_hist.root:/anatree");
      dir->GetObject("anatree",tree);

   }
   Init(tree);
}

looper::~looper()
{
   if (!fChain) return;
   delete fChain->GetCurrentFile();
}

Int_t looper::GetEntry(Long64_t entry)
{
// Read contents of entry.
   if (!fChain) return 0;
   return fChain->GetEntry(entry);
}
Long64_t looper::LoadTree(Long64_t entry)
{
// Set the environment to read one entry
   if (!fChain) return -5;
   Long64_t centry = fChain->LoadTree(entry);
   if (centry < 0) return centry;
   if (fChain->GetTreeNumber() != fCurrent) {
      fCurrent = fChain->GetTreeNumber();
      Notify();
   }
   return centry;
}

void looper::Init(TTree *tree)
{
   // The Init() function is called when the selector needs to initialize
   // a new tree or chain. Typically here the branch addresses and branch
   // pointers of the tree will be set.
   // It is normally not necessary to make changes to the generated
   // code, but the routine can be extended by the user if needed.
   // Init() will be called many times when running on PROOF
   // (once per file to be processed).

   // Set branch addresses and branch pointers
   if (!tree) return;
   fChain = tree;
   fCurrent = -1;
   fChain->SetMakeClass(1);

   fChain->SetBranchAddress("run", &run, &b_run);
   fChain->SetBranchAddress("subrun", &subrun, &b_subrun);
   fChain->SetBranchAddress("event", &event, &b_event);
   fChain->SetBranchAddress("evttime", &evttime, &b_evttime);
   fChain->SetBranchAddress("efield", efield, &b_efield);
   fChain->SetBranchAddress("t0", &t0, &b_t0);
   fChain->SetBranchAddress("ntracks_reco", &ntracks_reco, &b_ntracks_reco);
   fChain->SetBranchAddress("ntrkhits", &ntrkhits, &b_ntrkhits);
   fChain->SetBranchAddress("trkid", &trkid, &b_trkid);
   fChain->SetBranchAddress("trkstartx", &trkstartx, &b_trkstartx);
   fChain->SetBranchAddress("trkstarty", &trkstarty, &b_trkstarty);
   fChain->SetBranchAddress("trkstartz", &trkstartz, &b_trkstartz);
   fChain->SetBranchAddress("trkendx", &trkendx, &b_trkendx);
   fChain->SetBranchAddress("trkendy", &trkendy, &b_trkendy);
   fChain->SetBranchAddress("trkendz", &trkendz, &b_trkendz);
   fChain->SetBranchAddress("trkstartdcosx", &trkstartdcosx, &b_trkstartdcosx);
   fChain->SetBranchAddress("trkstartdcosy", &trkstartdcosy, &b_trkstartdcosy);
   fChain->SetBranchAddress("trkstartdcosz", &trkstartdcosz, &b_trkstartdcosz);
   fChain->SetBranchAddress("trkenddcosx", &trkenddcosx, &b_trkenddcosx);
   fChain->SetBranchAddress("trkenddcosy", &trkenddcosy, &b_trkenddcosy);
   fChain->SetBranchAddress("trkenddcosz", &trkenddcosz, &b_trkenddcosz);
   fChain->SetBranchAddress("trkx", &trkx, &b_trkx);
   fChain->SetBranchAddress("trky", &trky, &b_trky);
   fChain->SetBranchAddress("trkz", &trkz, &b_trkz);
   fChain->SetBranchAddress("trktheta_xz", &trktheta_xz, &b_trktheta_xz);
   fChain->SetBranchAddress("trktheta_yz", &trktheta_yz, &b_trktheta_yz);
   fChain->SetBranchAddress("trketa_xy", &trketa_xy, &b_trketa_xy);
   fChain->SetBranchAddress("trketa_zy", &trketa_zy, &b_trketa_zy);
   fChain->SetBranchAddress("trktheta", &trktheta, &b_trktheta);
   fChain->SetBranchAddress("trkphi", &trkphi, &b_trkphi);
   fChain->SetBranchAddress("trkd2", &trkd2, &b_trkd2);
   fChain->SetBranchAddress("trkdedx2", &trkdedx2, &b_trkdedx2);
   fChain->SetBranchAddress("trkdqdx", &trkdqdx, &b_trkdqdx);
   fChain->SetBranchAddress("trkpitch", &trkpitch, &b_trkpitch);
   fChain->SetBranchAddress("trkpitchHit", &trkpitchHit, &b_trkpitchHit);
   fChain->SetBranchAddress("trkkinE", &trkkinE, &b_trkkinE);
   fChain->SetBranchAddress("trkrange", &trkrange, &b_trkrange);
   fChain->SetBranchAddress("trkTPC", &trkTPC, &b_trkTPC);
   fChain->SetBranchAddress("trkplaneid", &trkplaneid, &b_trkplaneid);
   fChain->SetBranchAddress("trkresrg", &trkresrg, &b_trkresrg);
   fChain->SetBranchAddress("trkPosx", &trkPosx, &b_trkPosx);
   fChain->SetBranchAddress("trkPosy", &trkPosy, &b_trkPosy);
   fChain->SetBranchAddress("trkPosz", &trkPosz, &b_trkPosz);
   fChain->SetBranchAddress("trklen", &trklen, &b_trklen);
   fChain->SetBranchAddress("trklen_L", &trklen_L, &b_trklen_L);
   fChain->SetBranchAddress("trkdQdxSum", &trkdQdxSum, &b_trkdQdxSum);
   fChain->SetBranchAddress("trkdQdxAverage", &trkdQdxAverage, &b_trkdQdxAverage);
   fChain->SetBranchAddress("trkdEdxSum", &trkdEdxSum, &b_trkdEdxSum);
   fChain->SetBranchAddress("trkdEdxAverage", &trkdEdxAverage, &b_trkdEdxAverage);
   fChain->SetBranchAddress("trkMCTruthT0", &trkMCTruthT0, &b_trkMCTruthT0);
   fChain->SetBranchAddress("trkMCTruthTrackID", &trkMCTruthTrackID, &b_trkMCTruthTrackID);
   fChain->SetBranchAddress("nMCParticles", &nMCParticles, &b_nMCParticles);
   fChain->SetBranchAddress("trkid_MC", trkid_MC, &b_trkid_MC);
   fChain->SetBranchAddress("trkpdg_MC", trkpdg_MC, &b_trkpdg_MC);
   fChain->SetBranchAddress("nTPCHits_MC", nTPCHits_MC, &b_nTPCHits_MC);
   fChain->SetBranchAddress("StartInTPC_MC", StartInTPC_MC, &b_StartInTPC_MC);
   fChain->SetBranchAddress("EndInTPC_MC", EndInTPC_MC, &b_EndInTPC_MC);
   fChain->SetBranchAddress("trkMother_MC", trkMother_MC, &b_trkMother_MC);
   fChain->SetBranchAddress("trkNumDaughters_MC", trkNumDaughters_MC, &b_trkNumDaughters_MC);
   fChain->SetBranchAddress("trkFirstDaughter_MC", trkFirstDaughter_MC, &b_trkFirstDaughter_MC);
   fChain->SetBranchAddress("trkPrimary_MC", trkPrimary_MC, &b_trkPrimary_MC);
   fChain->SetBranchAddress("StartTime_MC", StartTime_MC, &b_StartTime_MC);
   fChain->SetBranchAddress("trkstartx_MC", trkstartx_MC, &b_trkstartx_MC);
   fChain->SetBranchAddress("trkstarty_MC", trkstarty_MC, &b_trkstarty_MC);
   fChain->SetBranchAddress("trkstartz_MC", trkstartz_MC, &b_trkstartz_MC);
   fChain->SetBranchAddress("trkendx_MC", trkendx_MC, &b_trkendx_MC);
   fChain->SetBranchAddress("trkendy_MC", trkendy_MC, &b_trkendy_MC);
   fChain->SetBranchAddress("trkendz_MC", trkendz_MC, &b_trkendz_MC);
   fChain->SetBranchAddress("trkenergy_MC", trkenergy_MC, &b_trkenergy_MC);
   fChain->SetBranchAddress("EnergyDeposited_MC", EnergyDeposited_MC, &b_EnergyDeposited_MC);
   fChain->SetBranchAddress("trkmom_MC", trkmom_MC, &b_trkmom_MC);
   fChain->SetBranchAddress("trkmom_XMC", trkmom_XMC, &b_trkmom_XMC);
   fChain->SetBranchAddress("trkmom_YMC", trkmom_YMC, &b_trkmom_YMC);
   fChain->SetBranchAddress("trkmom_ZMC", trkmom_ZMC, &b_trkmom_ZMC);
   fChain->SetBranchAddress("trkstartdoc_XMC", trkstartdoc_XMC, &b_trkstartdoc_XMC);
   fChain->SetBranchAddress("trkstartdoc_YMC", trkstartdoc_YMC, &b_trkstartdoc_YMC);
   fChain->SetBranchAddress("trkstartdoc_ZMC", trkstartdoc_ZMC, &b_trkstartdoc_ZMC);
   fChain->SetBranchAddress("mcpos_x", mcpos_x, &b_mcpos_x);
   fChain->SetBranchAddress("mcpos_y", mcpos_y, &b_mcpos_y);
   fChain->SetBranchAddress("mcpos_z", mcpos_z, &b_mcpos_z);
   fChain->SetBranchAddress("mcang_x", mcang_x, &b_mcang_x);
   fChain->SetBranchAddress("mcang_y", mcang_y, &b_mcang_y);
   fChain->SetBranchAddress("mcang_z", mcang_z, &b_mcang_z);
   fChain->SetBranchAddress("trktheta_xz_MC", trktheta_xz_MC, &b_trktheta_xz_MC);
   fChain->SetBranchAddress("trktheta_yz_MC", trktheta_yz_MC, &b_trktheta_yz_MC);
   fChain->SetBranchAddress("trktheta_MC", trktheta_MC, &b_trktheta_MC);
   fChain->SetBranchAddress("trkphi_MC", trkphi_MC, &b_trkphi_MC);
   fChain->SetBranchAddress("trketa_xy_MC", trketa_xy_MC, &b_trketa_xy_MC);
   fChain->SetBranchAddress("trketa_zy_MC", trketa_zy_MC, &b_trketa_zy_MC);
   fChain->SetBranchAddress("trkTPCLen_MC", trkTPCLen_MC, &b_trkTPCLen_MC);
   fChain->SetBranchAddress("nhits", &nhits, &b_nhits);
   fChain->SetBranchAddress("nhits2", &nhits2, &b_nhits2);
   fChain->SetBranchAddress("nclust", &nclust, &b_nclust);
   fChain->SetBranchAddress("hit_plane", hit_plane, &b_hit_plane);
   fChain->SetBranchAddress("hit_tpc", hit_tpc, &b_hit_tpc);
   fChain->SetBranchAddress("hit_wire", hit_wire, &b_hit_wire);
   fChain->SetBranchAddress("hit_channel", hit_channel, &b_hit_channel);
   fChain->SetBranchAddress("hit_peakT", hit_peakT, &b_hit_peakT);
   fChain->SetBranchAddress("hit_charge", hit_charge, &b_hit_charge);
   fChain->SetBranchAddress("hit_ph", hit_ph, &b_hit_ph);
   fChain->SetBranchAddress("hit_trkid", hit_trkid, &b_hit_trkid);
   fChain->SetBranchAddress("flash_total", &flash_total, &b_flash_total);
   fChain->SetBranchAddress("flash_time", flash_time, &b_flash_time);
   fChain->SetBranchAddress("flash_width", flash_width, &b_flash_width);
   fChain->SetBranchAddress("flash_abstime", flash_abstime, &b_flash_abstime);
   fChain->SetBranchAddress("flash_YCentre", flash_YCentre, &b_flash_YCentre);
   fChain->SetBranchAddress("flash_YWidth", flash_YWidth, &b_flash_YWidth);
   fChain->SetBranchAddress("flash_ZCentre", flash_ZCentre, &b_flash_ZCentre);
   fChain->SetBranchAddress("flash_ZWidth", flash_ZWidth, &b_flash_ZWidth);
   fChain->SetBranchAddress("flash_TotalPE", flash_TotalPE, &b_flash_TotalPE);
   fChain->SetBranchAddress("ntrigs", &ntrigs, &b_ntrigs);
   fChain->SetBranchAddress("trig_time", trig_time, &b_trig_time);
   fChain->SetBranchAddress("trig_id", trig_id, &b_trig_id);
   Notify();
}

Bool_t looper::Notify()
{
   // The Notify() function is called when a new file is opened. This
   // can be either for a new TTree in a TChain or when when a new TTree
   // is started when using PROOF. It is normally not necessary to make changes
   // to the generated code, but the routine can be extended by the
   // user if needed. The return value is currently not used.

   return kTRUE;
}

void looper::Show(Long64_t entry)
{
// Print contents of entry.
// If entry is not specified, print current entry
   if (!fChain) return;
   fChain->Show(entry);
}
Int_t looper::Cut(Long64_t entry)
{
// This function may be called from Loop.
// returns  1 if entry is accepted.
// returns -1 otherwise.
   return 1;
}
#endif // #ifdef looper_cxx

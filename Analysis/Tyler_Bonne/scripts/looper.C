#define looper_cxx
#include "looper.h"
#include <TH2.h>
#include <TStyle.h>
#include <TCanvas.h>

void looper::Loop()
{
//   In a ROOT session, you can do:
//      Root > .L looper.C
//      Root > looper t
//      Root > t.GetEntry(12); // Fill t data members with entry number 12
//      Root > t.Show();       // Show values of entry 12
//      Root > t.Show(16);     // Read and show values of entry 16
//      Root > t.Loop();       // Loop on all entries
//

//     This is the loop skeleton where:
//    jentry is the global entry number in the chain
//    ientry is the entry number in the current Tree
//  Note that the argument to GetEntry must be:
//    jentry for TChain::GetEntry
//    ientry for TTree::GetEntry and TBranch::GetEntry
//
//       To read only selected branches, Insert statements like:
// METHOD1:
//    fChain->SetBranchStatus("*",0);  // disable all branches
//    fChain->SetBranchStatus("branchname",1);  // activate branchname
// METHOD2: replace line
//    fChain->GetEntry(jentry);       //read all branches
//by  b_branchname->GetEntry(ientry); //read only this branch

   if (fChain == 0) return;

   Long64_t nentries = fChain->GetEntriesFast();

   Long64_t nbytes = 0, nb = 0;

//I begin by declaring all of the histograms I will be filling here, so that in the looping segment I can just fill them as it runs:

	//Charge histogram
	TH1 *charge = new TH1D("charge","Charge Per Particle",100,0,400000); 
	
	//Log(charge) histogram
	TH1 *logcharge = new TH1D("logcharge","Log of charge",100,0,7);
	
	//Track length histrogram
	TH1 *trklenhist = new TH1D("trklenhist","Total track length per particle",100,0,300);

	//PE of each event histogram
	TH1 *PEperevent = new TH1D("PEperevent","PE of each event",100,0,2000);


	//PE vs track length TH2:
	TH2 *PEvsTrklen = new TH2D("PEvsTrklen","PE versus track length",100,0,400,100,0,1800);
	PEvsTrklen->GetXaxis()->SetTitle("Track length (cm)");
	PEvsTrklen->GetXaxis()->CenterTitle();
	PEvsTrklen->GetYaxis()->SetTitle("PE");
	PEvsTrklen->GetYaxis()->CenterTitle();

	//PE vs vertex TH2:
	TH2 *PEvsVertex = new TH2D("PEvsVertex","PE versus Vertex",100,-80,270,100,0,1800);
	PEvsVertex->GetXaxis()->SetTitle("Vertex (cm)");
	PEvsVertex->GetXaxis()->CenterTitle();
	PEvsVertex->GetYaxis()->SetTitle("PE");
	PEvsVertex->GetYaxis()->CenterTitle();

	//Charge vs vertex TH2:
	TH2 *ChargevsVertex = new TH2D("ChargevsVertex","Charge versus Vertex",100,-80,270,100,0,400000);
	ChargevsVertex->GetXaxis()->SetTitle("Vertex (cm)");
	ChargevsVertex->GetXaxis()->CenterTitle();
	ChargevsVertex->GetYaxis()->SetTitle("Charge");
	ChargevsVertex->GetYaxis()->CenterTitle();

	//For counting instances of 0 in hit_charge
	int zerocharge = 0;

	//Arrays for the TGraph
	double PEarray[1000];
	double trklenarray[1000];
	double vertexarray[1000];
	double chargearray[1000];


//Here begins the looping over events, in the for loop below. Everything in this section is meant to be done for each event, which are called jentry:

   for (Long64_t jentry=0; jentry<nentries;jentry++) {
      Long64_t ientry = LoadTree(jentry);
      if (ientry < 0) break;
      nb = fChain->GetEntry(jentry);   nbytes += nb;
      // if (Cut(ientry) < 0) continue; <-This is commented out by default

	//This for loop determines which particle in each jentry is the primary particle, by using an if statement. It then allows the value j to determine which nMCParticles element is the primary particle for each jentry
	for (int j=0;j<nMCParticles;j++){
	if (trkPrimary_MC[j]==1){
	vertexarray[jentry]=trkstartx_MC[j];
	

	//This is where I do all charge-related work, the below  if statement checks to make sure that there are hits before allowing the program to continue. 
	//You will notice similar statements for all sections, this ensures no non-hit events are counted to ensure proper statistics
	if (nhits2>0){
	//Calculate total charge, fill histogram
	double ttlcharge = 0;
	for (int i=0;i<nhits2;i++){
	ttlcharge+=hit_charge[i];
	}
	charge->Fill(ttlcharge);
	double logofcharge = log10(ttlcharge);
	logcharge->Fill(logofcharge);
	chargearray[jentry]=ttlcharge;
	//Checking low charge values
	if (ttlcharge==0){
	//cout << ttlcharge << endl; 
	zerocharge++; }}

	
	//Track length calculations:
	if (nMCParticles>0){
	//Calculate total track length, fill histogram
	double ttltrklen = 0;
	for (int i=0;i<nMCParticles;i++){
	ttltrklen+=trkTPCLen_MC[i];
	}
	trklenhist->Fill(ttltrklen);
	trklenarray[jentry]=ttltrklen;}


	//Calculate total PE, fill histogram	
	if (flash_total>0){
	double ttlPE = 0;
	for (int i=0;i<flash_total;i++){
	ttlPE+=flash_TotalPE[i];
	}
	PEperevent->Fill(ttlPE);
	PEarray[jentry]=ttlPE;}


////////////////////////////////////////////
//Filling the TH2Ds:                      //
////////////////////////////////////////////
	
	PEvsTrklen->Fill(ttltrklen,ttlPE);
	PEvsVertex->Fill(trkstartx_MC[j],ttlPE);
	ChargevsVertex->Fill(trkstartx_MC[j],ttlcharge);
	
	//Below is a bit of code that allows comparison of each MC track vertex with its corresponding energy for debugging purposes
	//Checking entries in trkstartx_MC
	/*for (int i=0;i<nMCParticles;i++){
	if (trkstartx_MC[i]>0){
	cout << "Event " << jentry << " has trkstartx_MC " << trkstartx_MC[i] << " with energy " << trkenergy_MC[i] << endl;
	}
	}*/
	}
}
   }

	//Declare output file so all generated TObjects can be stored and examined
	TFile *outfile = new TFile("loopoutput.root","recreate");


/*Making TGraphs, commented out so they can be TH2Ds instead, but still usable if necessary
	//TGraph for PE vs trklen
	TGraph *pevstrklen = new TGraph(1000,trklenarray,PEarray);
	
	//pevsx->Draw();
	pevstrklen->SetMarkerStyle(20);
	pevstrklen->GetXaxis()->SetTitle("Track length (cm)");
	pevstrklen->GetXaxis()->CenterTitle();
	pevstrklen->GetYaxis()->SetTitle("PE");
	pevstrklen->GetYaxis()->CenterTitle();
	pevstrklen->SetTitle("Track length vs. PE");
	pevstrklen->Write("pevstrklen");
	//anatree->Write(pevstrklen);
	//anatree->Append(pevstrklen);
	//pevstrklen->Write();
	//pevsx->Draw("ap");


	//TGraph for PE vs trklen
	TGraph *pevsvertex = new TGraph(1000,vertexarray,PEarray);

	pevsvertex->SetMarkerStyle(20);
	pevsvertex->GetXaxis()->SetTitle("Vertex (cm)");
	pevsvertex->GetXaxis()->CenterTitle();
	pevsvertex->GetYaxis()->SetTitle("PE");
	pevsvertex->GetYaxis()->CenterTitle();
	pevsvertex->SetTitle("Vertex vs. PE");
	pevsvertex->Write("pevsvertex");
	//anatree->Append(pevsvertex);
	//pevsvertex->Write();

	//TGraph for PE vs trklen
	TGraph *chargevsvertex = new TGraph(1000,vertexarray,chargearray);

	chargevsvertex->SetMarkerStyle(20);
	chargevsvertex->GetXaxis()->SetTitle("Vertex (cm)");
	chargevsvertex->GetXaxis()->CenterTitle();
	chargevsvertex->GetYaxis()->SetTitle("Charge");
	chargevsvertex->GetYaxis()->CenterTitle();
	chargevsvertex->SetTitle("Vertex vs. Charge");
	chargevsvertex->Write("chargevsvertex");
	//anatree->Append(chargevsvertex);
	//chargevsvertex->Write();
	//cout << zerocharge << endl;*/
//End TGraph section

	
	
	//Write all objects to outfile
	charge->Write("charge");
	logcharge->Write("logcharge");
	trklenhist->Write("trklenhist");
	PEperevent->Write("PEperevent");
	PEvsTrklen->Write("PEvsTrklen");
	PEvsVertex->Write("PEvsVertex");
	ChargevsVertex->Write("ChargevsVertex");
	



}

#include "xAODAnaHelpers/ClusterHists.h"

#include <math.h>

#include "xAODAnaHelpers/tools/ReturnCheck.h"

ClusterHists :: ClusterHists (std::string name, std::string detailStr) :
  HistogramManager(name, detailStr)
{
}

ClusterHists :: ~ClusterHists () {}

StatusCode ClusterHists::initialize() {

  // These plots are always made
  m_ccl_n   = book(m_name, "n", "cluster multiplicity", 80, 0, 800);
  m_ccl_e   = book(m_name, "e", "cluster e [GeV]", 300, -200, 1000);
  m_ccl_pt  = book(m_name, "pt", "cluster pt [GeV]", 300, -200, 1000);
  m_ccl_eta = book(m_name, "eta", "cluster #eta", 80, -4, 4);
  m_ccl_phi = book(m_name, "phi", "cluster #phi", 120, -TMath::Pi(), TMath::Pi());

  // 2D plots 
  m_ccl_eta_vs_phi = book(m_name, "eta_vs_phi", "cluster #phi", 120, -TMath::Pi(), TMath::Pi(), "cluster #eta", 80, -4, 4);
  m_ccl_e_vs_eta   = book(m_name, "e_vs_eta", "cluster #eta", 80, -4, 4, "cluster e [GeV]", 300, -5, 45);
  m_ccl_e_vs_phi   = book(m_name, "e_vs_phi", "cluster #phi", 120, -TMath::Pi(), TMath::Pi(), "cluster e [GeV]", 300, -5, 45);
  m_ccl_pt_vs_eta   = book(m_name, "pt_vs_eta", "cluster #eta", 80, -4, 4, "cluster pt [GeV]", 300, -5, 45);
  m_ccl_pt_vs_phi   = book(m_name, "pt_vs_phi", "cluster #phi", 120, -TMath::Pi(), TMath::Pi(), "cluster pt [GeV]", 300, -5, 45);


  if(m_detailStr.find("eta") != std::string::npos){
    m_ccl_e_0_0p8   = book(m_name, "e_0_0p8", "cluster e [GeV]", 300, -200, 1000);
    m_ccl_pt_0_0p8  = book(m_name, "pt_0_0p8", "cluster pt [GeV]", 300, -200, 1000);
    m_ccl_e_0_1p7   = book(m_name, "e_0_1p7", "cluster e [GeV]", 300, -200, 1000);
    m_ccl_pt_0_1p7  = book(m_name, "pt_0_1p7", "cluster pt [GeV]", 300, -200, 1000);
    m_ccl_e_1p7_5   = book(m_name, "e_1p7_5", "cluster e [GeV]", 300, -200, 1000);
    m_ccl_pt_1p7_5  = book(m_name, "pt_1p7_5", "cluster pt [GeV]", 300, -200, 1000);
    m_ccl_e_2p5_5   = book(m_name, "e_2p5_5", "cluster e [GeV]", 300, -200, 1000);
    m_ccl_pt_2p5_5  = book(m_name, "pt_2p5_5", "cluster pt [GeV]", 300, -200, 1000);
    m_ccl_e_3p5_5   = book(m_name, "e_3p5_5", "cluster e [GeV]", 300, -200, 1000);
    m_ccl_pt_3p5_5  = book(m_name, "pt_3p5_5", "cluster pt [GeV]", 300, -200, 1000);
  }


  // if worker is passed to the class add histograms to the output
  return StatusCode::SUCCESS;
}

StatusCode ClusterHists::execute( const xAOD::CaloClusterContainer* ccls, float eventWeight ) {
  xAOD::CaloClusterContainer::const_iterator ccl_itr = ccls->begin();
  xAOD::CaloClusterContainer::const_iterator ccl_end = ccls->end();
  for( ; ccl_itr != ccl_end; ++ccl_itr ) {
    RETURN_CHECK("ClusterHists::execute()", this->execute( (*ccl_itr), eventWeight ), "");
  }

  m_ccl_n -> Fill( ccls->size(), eventWeight );

  return StatusCode::SUCCESS;
}

StatusCode ClusterHists::execute( const xAOD::CaloCluster* ccl, float eventWeight ) {

  //basic
  float cclE   = ccl->e()/1e3;
  float cclPt  = ccl->pt()/1e3;
  float cclEta = ccl->eta();
  float cclPhi = ccl->phi();

  m_ccl_e          -> Fill( cclE,   eventWeight );
  m_ccl_pt         -> Fill( cclPt,  eventWeight );
  m_ccl_eta        -> Fill( cclEta, eventWeight );
  m_ccl_phi        -> Fill( cclPhi, eventWeight );

  // 2D plots 
  m_ccl_eta_vs_phi -> Fill( cclPhi, cclEta,  eventWeight );
  m_ccl_e_vs_eta   -> Fill( cclEta, cclE,    eventWeight );
  m_ccl_e_vs_phi   -> Fill( cclPhi, cclE,    eventWeight );
  m_ccl_pt_vs_eta  -> Fill( cclEta, cclPt,   eventWeight );
  m_ccl_pt_vs_phi  -> Fill( cclPhi, cclPt,   eventWeight );
  

  if(m_detailStr.find("eta") != std::string::npos){
    if(fabs(cclEta) < 0.8){
      m_ccl_e_0_0p8          -> Fill( cclE,   eventWeight );
      m_ccl_pt_0_0p8         -> Fill( cclPt,  eventWeight );
    }
    if(fabs(cclEta) < 1.7){
      m_ccl_e_0_1p7          -> Fill( cclE,   eventWeight );
      m_ccl_pt_0_1p7         -> Fill( cclPt,  eventWeight );
    }
    if(fabs(cclEta) > 1.7){
      m_ccl_e_1p7_5          -> Fill( cclE,   eventWeight );
      m_ccl_pt_1p7_5         -> Fill( cclPt,  eventWeight );
    }
    if(fabs(cclEta) > 2.5){
      m_ccl_e_2p5_5          -> Fill( cclE,   eventWeight );
      m_ccl_pt_2p5_5         -> Fill( cclPt,  eventWeight );
    }
    if(fabs(cclEta) > 3.5){
      m_ccl_e_3p5_5          -> Fill( cclE,   eventWeight );
      m_ccl_pt_3p5_5         -> Fill( cclPt,  eventWeight );
    }
  }

  return StatusCode::SUCCESS;

}

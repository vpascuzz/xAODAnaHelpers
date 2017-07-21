/*************************************************
 *
 * Interface to ASG overlap removal tool
 ( applying recommendations from Harmonisation TF ).
 *
 * M. Milesi (marco.milesi@cern.ch)
 * J. Dandoy
 *
 ************************************************/

// c++ include(s):
#include <iostream>
#include <sstream>

// EL include(s):
#include <EventLoop/Job.h>
#include <EventLoop/StatusCode.h>
#include <EventLoop/Worker.h>

// EDM include(s):
#include "xAODEgamma/ElectronContainer.h"
#include "xAODEgamma/Electron.h"
#include "xAODEgamma/PhotonContainer.h"
#include "xAODEgamma/Photon.h"
#include "xAODMuon/MuonContainer.h"
#include "xAODMuon/Muon.h"
#include "xAODJet/JetContainer.h"
#include "xAODJet/Jet.h"
#include "xAODTau/TauJetContainer.h"
#include "xAODTau/TauJet.h"
#include "AthContainers/ConstDataVector.h"
#include "xAODCore/ShallowCopy.h"

// package include(s):
#include "xAODAnaHelpers/OverlapRemover.h"
#include "xAODAnaHelpers/HelperFunctions.h"
#include "xAODAnaHelpers/HelperClasses.h"

using HelperClasses::ToolName;

// this is needed to distribute the algorithm to the workers
ClassImp(OverlapRemover)


OverlapRemover :: OverlapRemover () :
    Algorithm("OverlapRemover")
{
}

EL::StatusCode OverlapRemover :: setupJob (EL::Job& job)
{
  // Here you put code that sets up the job on the submission object
  // so that it is ready to work with your algorithm, e.g. you can
  // request the D3PDReader service or add output files.  Any code you
  // put here could instead also go into the submission script.  The
  // sole advantage of putting it here is that it gets automatically
  // activated/deactivated when you add/remove the algorithm from your
  // job, which may or may not be of value to you.

  ANA_MSG_INFO( "Calling setupJob");

  job.useXAOD ();
  xAOD::Init( "OverlapRemover" ).ignore(); // call before opening first file

  return EL::StatusCode::SUCCESS;
}



EL::StatusCode OverlapRemover :: histInitialize ()
{
  // Here you do everything that needs to be done at the very
  // beginning on each worker node, e.g. create histograms and output
  // trees.  This method gets called before any input files are
  // connected.
  ANA_CHECK( xAH::Algorithm::algInitialize());
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode OverlapRemover :: fileExecute ()
{
  // Here you do everything that needs to be done exactly once for every
  // single file, e.g. collect a list of all lumi-blocks processed
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode OverlapRemover :: changeInput (bool /*firstFile*/)
{
  // Here you do everything you need to do when we change input files,
  // e.g. resetting branch addresses on trees.  If you are using
  // D3PDReader or a similar service this method is not needed.
  return EL::StatusCode::SUCCESS;
}



EL::StatusCode OverlapRemover :: initialize ()
{
  // Here you do everything that you need to do after the first input
  // file has been connected and before the first event is processed,
  // e.g. create additional histograms based on which variables are
  // available in the input files.  You can also create all of your
  // histograms and trees in here, but be aware that this method
  // doesn't get called if no events are processed.  So any objects
  // you create here won't be available in the output if you have no
  // input events.

  ANA_MSG_INFO( "Initializing OverlapRemover Interface... ");

  if ( setCutFlowHist() == EL::StatusCode::FAILURE ) {
    ANA_MSG_ERROR( "Failed to setup cutflow histograms. Exiting." );
    return EL::StatusCode::FAILURE;
  }

  m_event = wk()->xaodEvent();
  m_store = wk()->xaodStore();

  ANA_MSG_INFO( "Number of events in file: " << m_event->getEntries() );

  if ( m_inContainerName_Jets.empty() ) {
    ANA_MSG_ERROR( "InputContainerJets is empty! Must have it to perform Overlap Removal! Exiting.");
    return EL::StatusCode::FAILURE;
  }

  if ( !m_inContainerName_Electrons.empty() ) { m_useElectrons = true; }
  if ( !m_inContainerName_Muons.empty() )     { m_useMuons     = true; }
  if ( !m_inContainerName_Taus.empty() )      { m_useTaus      = true; }
  if ( !m_inContainerName_Photons.empty() )   { m_usePhotons   = true; }

  m_outAuxContainerName_Electrons   = m_outContainerName_Electrons + "Aux."; // the period is very important!
  m_outAuxContainerName_Muons       = m_outContainerName_Muons + "Aux.";     // the period is very important!
  m_outAuxContainerName_Jets        = m_outContainerName_Jets + "Aux.";      // the period is very important!
  m_outAuxContainerName_Photons     = m_outContainerName_Photons + "Aux.";   // the period is very important!
  m_outAuxContainerName_Taus        = m_outContainerName_Taus + "Aux.";      // the period is very important!

  if ( setCounters() == EL::StatusCode::FAILURE ) {
    ANA_MSG_ERROR( "Failed to properly set event/object counters. Exiting." );
    return EL::StatusCode::FAILURE;
  }

  // initialize ASG overlap removal tool
  const std::string selected_label = ( m_useSelected ) ? "passSel" : "";  // set with decoration flag you use for selected objects if want to consider only selected objects in OR, otherwise it will perform OR on all objects

  //Set Flags for recommended overlap procedures
  ORUtils::ORFlags orFlags("OverlapRemovalTool", selected_label, "passOR");

  orFlags.outputPassValue     = true;
  orFlags.linkOverlapObjects  = m_linkOverlapObjects;
  orFlags.bJetLabel           = m_bTagWP;
  orFlags.boostedLeptons      = m_useBoostedLeptons;
  orFlags.doEleEleOR          = m_doEleEleOR;

  orFlags.doJets      = true;
  orFlags.doMuons     = m_useMuons;
  orFlags.doElectrons = m_useElectrons;
  orFlags.doTaus      = m_useTaus;
  orFlags.doPhotons   = m_usePhotons;
  orFlags.doFatJets   = false;

  ANA_CHECK( ORUtils::recommendedTools(orFlags, m_ORToolbox));
  ANA_CHECK( m_ORToolbox.initialize());
  ANA_MSG_INFO( "OverlapRemover Interface succesfully initialized!" );

  return EL::StatusCode::SUCCESS;
}


EL::StatusCode OverlapRemover :: execute ()
{
  // Here you do everything that needs to be done on every single
  // events, e.g. read input variables, apply cuts, and fill
  // histograms and trees.  This is where most of your actual analysis
  // code will go.

  ANA_MSG_DEBUG("Applying Overlap Removal... ");

  m_numEvent++;

  // get the collections from TEvent or TStore
  const xAOD::ElectronContainer* inElectrons (nullptr);
  const xAOD::MuonContainer* inMuons         (nullptr);
  const xAOD::JetContainer* inJets           (nullptr);
  const xAOD::PhotonContainer* inPhotons     (nullptr);
  const xAOD::TauJetContainer* inTaus        (nullptr);

  // syst container name
  m_vecOutContainerNames = new std::vector< std::string >;

  // --------------------------------------------------------------------------------------------
  //
  // always run the nominal case
  executeOR(inElectrons, inMuons, inJets, inPhotons, inTaus);

  // look what do we have in TStore
  if(msgLvl(MSG::VERBOSE)) m_store->print();

  // -----------------------------------------------------------------------------------------------
  //
  // if at least one of the m_inputAlgo* is not empty, and there's at least one non-empty syst name,
  // then perform OR for every non-empty systematic set

  // ****************** //
  //      Electrons     //
  // ****************** //

  if ( !m_inputAlgoElectrons.empty() ) {

    // -----------------------
    //
    // get the systematic sets:

    // get vector of string giving the syst names (rememeber: 1st element is a blank string: nominal case!)
    std::vector<std::string>* systNames_el(nullptr);
    ANA_CHECK( HelperFunctions::retrieve(systNames_el, m_inputAlgoElectrons, 0, m_store, msg()) );

    if ( HelperFunctions::found_non_dummy_sys(systNames_el) ) {
      executeOR(inElectrons, inMuons, inJets, inPhotons, inTaus,  ELSYST, *systNames_el);
    }

  }

  // **************** //
  //      Muons       //
  // **************** //

  if ( !m_inputAlgoMuons.empty() ) {

    // -----------------------
    //
    // get the systematic sets:

    // get vector of string giving the syst names (rememeber: 1st element is a blank string: nominal case!)
    std::vector<std::string>* systNames_mu(nullptr);
    ANA_CHECK( HelperFunctions::retrieve(systNames_mu, m_inputAlgoMuons, 0, m_store, msg()) );

    if ( HelperFunctions::found_non_dummy_sys(systNames_mu) ) {
      executeOR(inElectrons, inMuons, inJets, inPhotons, inTaus,  MUSYST, *systNames_mu);
    }

  }

  // **************** //
  //       Jets       //
  // **************** //

  if ( !m_inputAlgoJets.empty() ) {

    // -----------------------
    //
    // get the systematic sets:

    // get vector of string giving the syst names (rememeber: 1st element is a blank string: nominal case!)
    std::vector<std::string>* systNames_jet;
    ANA_CHECK( HelperFunctions::retrieve(systNames_jet, m_inputAlgoJets, 0, m_store, msg()) );

    if ( HelperFunctions::found_non_dummy_sys(systNames_jet) ) {
      executeOR(inElectrons, inMuons, inJets, inPhotons, inTaus,  JETSYST, *systNames_jet);
    }

  }

  // **************** //
  //     Photons      //
  // **************** //

  if ( !m_inputAlgoPhotons.empty() ) {

    // -----------------------
    //
    // get the systematic sets:

    // get vector of string giving the syst names (rememeber: 1st element is a blank string: nominal case!)
    std::vector<std::string>* systNames_photon;
    ANA_CHECK( HelperFunctions::retrieve(systNames_photon, m_inputAlgoPhotons, 0, m_store, msg()) );

    executeOR(inElectrons, inMuons, inJets, inPhotons, inTaus,  PHSYST, *systNames_photon);

  }

  // **************** //
  //       Taus       //
  // **************** //

  if ( !m_inputAlgoTaus.empty() ) {

    // -----------------------
    //
    // get the systematic sets:

    // get vector of string giving the syst names (rememeber: 1st element is a blank string: nominal case!)
    std::vector<std::string>* systNames_tau;
    ANA_CHECK( HelperFunctions::retrieve(systNames_tau, m_inputAlgoTaus, 0, m_store, msg()) );

    executeOR(inElectrons, inMuons, inJets, inPhotons, inTaus, TAUSYST, *systNames_tau);

  }

  // save list of systs that should be considered down stream
  ANA_CHECK( m_store->record( m_vecOutContainerNames, m_outputAlgoSystNames));

  // look what do we have in TStore
  if(msgLvl(MSG::VERBOSE)) m_store->print();

  return EL::StatusCode::SUCCESS;

}

EL::StatusCode OverlapRemover :: postExecute ()
{
  // Here you do everything that needs to be done after the main event
  // processing.  This is typically very rare, particularly in user
  // code.  It is mainly used in implementing the NTupleSvc.

  ANA_MSG_DEBUG("Calling postExecute");

  return EL::StatusCode::SUCCESS;
}


EL::StatusCode OverlapRemover :: finalize ()
{
  // This method is the mirror image of initialize(), meaning it gets
  // called after the last event has been processed on the worker node
  // and allows you to finish up any objects you created in
  // initialize() before they are written to disk.  This is actually
  // fairly rare, since this happens separately for each worker node.
  // Most of the time you want to do your post-processing on the
  // submission node after all your histogram outputs have been
  // merged.  This is different from histFinalize() in that it only
  // gets called on worker nodes that processed input events.

  ANA_MSG_INFO( "Deleting tool instances...");

  return EL::StatusCode::SUCCESS;
}



EL::StatusCode OverlapRemover :: histFinalize ()
{
  // This method is the mirror image of histInitialize(), meaning it
  // gets called after the last event has been processed on the worker
  // node and allows you to finish up any objects you created in
  // histInitialize() before they are written to disk.  This is
  // actually fairly rare, since this happens separately for each
  // worker node.  Most of the time you want to do your
  // post-processing on the submission node after all your histogram
  // outputs have been merged.  This is different from finalize() in
  // that it gets called on all worker nodes regardless of whether
  // they processed input events.

  ANA_MSG_INFO( "Calling histFinalize");
  ANA_CHECK( xAH::Algorithm::algFinalize());
  return EL::StatusCode::SUCCESS;
}


EL::StatusCode OverlapRemover :: fillObjectCutflow (const xAOD::IParticleContainer* objCont, const std::string& overlapFlag, const std::string& selectFlag)
{
  SG::AuxElement::ConstAccessor<char> selectAcc(selectFlag);
  SG::AuxElement::ConstAccessor<char> overlapAcc(overlapFlag);
  static SG::AuxElement::ConstAccessor< ElementLink<xAOD::IParticleContainer> > objLinkAcc("overlapObject");

  for ( auto obj_itr : *(objCont) ) {

    std::string type;

    // Safety check
    //
    if ( !overlapAcc.isAvailable( *obj_itr ) ) {
      ANA_MSG_ERROR( "Overlap decoration missing for this object");
      return EL::StatusCode::FAILURE;
    }
    switch(obj_itr->type()) {
      case xAOD::Type::Electron:
        type = "electron";
        if (!overlapAcc( *obj_itr ))
          m_el_cutflowHist_1->Fill( m_el_cutflow_OR_cut, 1 );
        break;
      case xAOD::Type::Muon:
        if (!overlapAcc( *obj_itr ))
          m_mu_cutflowHist_1->Fill( m_mu_cutflow_OR_cut, 1 );
        type = "muon";
        break;
      case xAOD::Type::Jet:
        if (!overlapAcc( *obj_itr ))
          m_jet_cutflowHist_1->Fill( m_jet_cutflow_OR_cut, 1 );
        type = "jet";
        break;
      case xAOD::Type::Photon:
        if (!overlapAcc( *obj_itr ))
          m_ph_cutflowHist_1->Fill( m_ph_cutflow_OR_cut, 1 );
        type = "photon";
        break;
      case xAOD::Type::Tau:
        if (!overlapAcc( *obj_itr ))
          m_tau_cutflowHist_1->Fill( m_tau_cutflow_OR_cut, 1 );
        type = "tau";
        break;
      default:
        ANA_MSG_ERROR("Unsupported object");
        return EL::StatusCode::FAILURE;
        break;
    }

    int isBTagged = 0;
    if ( !m_bTagWP.empty() ) {
      SG::AuxElement::Decorator< char > isBTag( m_bTagWP );
      if( isBTag.isAvailable( *obj_itr ) && isBTag(*obj_itr)==true ) isBTagged = 1;
    }

    if(selectAcc.isAvailable(*obj_itr)){
      ANA_MSG_DEBUG( type << " pt " << obj_itr->pt()/1e3 << " eta " << obj_itr->eta() << " phi " << obj_itr->phi() << " btagged " << isBTagged << " selected " << selectAcc(*obj_itr) << " passesOR  " << overlapAcc( *obj_itr ) );
    } else {
      ANA_MSG_DEBUG( type << " pt " << obj_itr->pt()/1e3 << " eta " << obj_itr->eta() << " phi " << obj_itr->phi() << " btagged " << isBTagged << " selected N/A" << " passesOR  " << overlapAcc( *obj_itr ) );
    }
    // Check for overlap object link
    if ( objLinkAcc.isAvailable( *obj_itr ) && objLinkAcc( *obj_itr ).isValid() ) {
      const xAOD::IParticle* overlapObj = *objLinkAcc( *obj_itr );
      std::stringstream ss_or; ss_or << overlapObj->type();
      ANA_MSG_DEBUG( " Overlap: type " << ss_or.str() << " pt " << overlapObj->pt()/1e3);
    }

  }

  return EL::StatusCode::SUCCESS;

}

// a nice override to handle nominal cases simply
EL::StatusCode OverlapRemover :: executeOR(  const xAOD::ElectronContainer* inElectrons, const xAOD::MuonContainer* inMuons, const xAOD::JetContainer* inJets,
               const xAOD::PhotonContainer* inPhotons,   const xAOD::TauJetContainer* inTaus)
{
  std::vector<std::string> nominalVec{""};
  return executeOR(inElectrons, inMuons, inJets, inPhotons, inTaus, NOMINAL, nominalVec);
}

EL::StatusCode OverlapRemover :: executeOR(  const xAOD::ElectronContainer* inElectrons, const xAOD::MuonContainer* inMuons, const xAOD::JetContainer* inJets,
               const xAOD::PhotonContainer* inPhotons,   const xAOD::TauJetContainer* inTaus,
               SystType syst_type, const std::vector<std::string>& sysVec)
{

  // instantiate output container(s)
  //
  ConstDataVector<xAOD::ElectronContainer> *selectedElectrons   (nullptr);
  ConstDataVector<xAOD::MuonContainer>     *selectedMuons (nullptr);
  ConstDataVector<xAOD::JetContainer>      *selectedJets  (nullptr);
  ConstDataVector<xAOD::PhotonContainer>   *selectedPhotons (nullptr);
  ConstDataVector<xAOD::TauJetContainer>   *selectedTaus  (nullptr);


  // get a nice name for the current syst_type for warning
  std::string syst_type_name = "nominal";
  switch ( static_cast<int>(syst_type) ) {
    case NOMINAL:   syst_type_name = "nominal"; break;
    case ELSYST:    syst_type_name = "electron"; break;
    case MUSYST:    syst_type_name = "muon"; break;
    case JETSYST:   syst_type_name = "jet"; break;
    case PHSYST:    syst_type_name = "photon"; break;
    case TAUSYST:   syst_type_name = "tau"; break;
    default:
      {
        ANA_MSG_ERROR("Unknown systematics type. Aborting");
        return EL::StatusCode::FAILURE;
      }
      break;
  }

  // debugging purposes
  ANA_MSG_DEBUG("Doing " << syst_type_name << " systematics");
  if(syst_type != NOMINAL){
    ANA_MSG_DEBUG("will consider the following systematics:");
    for (const auto& systName : sysVec ){ ANA_MSG_DEBUG("\t " << systName); }
  }

  // if any of the nominal containers are not found for a given systematic type
  //    we must just return success and warn that OR will not be done for that case
  bool nomContainerNotFound(false);

  // first, retrieve all nominal containers we need

  if( m_useElectrons && syst_type != ELSYST ) {
    if ( m_store->contains<ConstDataVector<xAOD::ElectronContainer> >(m_inContainerName_Electrons) ) {
      ANA_CHECK( HelperFunctions::retrieve(inElectrons, m_inContainerName_Electrons, m_event, m_store, msg()) );
    } else {
      nomContainerNotFound = true;
      ANA_MSG_ERROR( "Could not find nominal container " << m_inContainerName_Electrons << " in xAOD::TStore. Overlap Removal will not be done for the " << syst_type_name << " systematic case...");
    }
  }

  if( m_useMuons && syst_type != MUSYST ) {
    if ( m_store->contains<ConstDataVector<xAOD::MuonContainer> >(m_inContainerName_Muons) ) {
      ANA_CHECK( HelperFunctions::retrieve(inMuons, m_inContainerName_Muons, m_event, m_store, msg()) );
    } else {
      nomContainerNotFound = true;
      ANA_MSG_ERROR( "Could not find nominal container " << m_inContainerName_Muons << " in xAOD::TStore. Overlap Removal will not be done for the " << syst_type_name << " systematic case...");
    }
  }

  if( syst_type != JETSYST ){
    if ( m_store->contains<ConstDataVector<xAOD::JetContainer> >(m_inContainerName_Jets) ) {
      ANA_CHECK( HelperFunctions::retrieve(inJets, m_inContainerName_Jets, m_event, m_store, msg()) );
    } else {
      nomContainerNotFound = true;
      ANA_MSG_ERROR( "Could not find nominal container " << m_inContainerName_Jets << " in xAOD::TStore. Overlap Removal will not be done for the " << syst_type_name << " systematic case...");
    }
  }

  if ( m_usePhotons && syst_type != PHSYST ) {
    if ( m_store->contains<ConstDataVector<xAOD::PhotonContainer> >(m_inContainerName_Photons) ) {
      ANA_CHECK( HelperFunctions::retrieve(inPhotons, m_inContainerName_Photons, m_event, m_store, msg()) );
    } else {
      nomContainerNotFound = true;
      ANA_MSG_ERROR( "Could not find nominal container " << m_inContainerName_Photons << " in xAOD::TStore. Overlap Removal will not be done for the " << syst_type_name << " systematic case...");
    }
  }

  if ( m_useTaus && syst_type != TAUSYST ) {
    if ( m_store->contains<ConstDataVector<xAOD::TauJetContainer> >(m_inContainerName_Taus) ) {
      ANA_CHECK( HelperFunctions::retrieve(inTaus, m_inContainerName_Taus, m_event, m_store, msg()) );
    } else {
      nomContainerNotFound = true;
      ANA_MSG_ERROR( "Could not find nominal container " << m_inContainerName_Taus << " in xAOD::TStore. Overlap Removal will not be done for the " << syst_type_name << " systematic case...");
    }
  }

  if( nomContainerNotFound ) { return EL::StatusCode::FAILURE; }

  bool doElectrons(m_useElectrons || syst_type == ELSYST);
  bool doMuons(m_useMuons || syst_type == MUSYST);
  //bool doJets(true); /// always do jets
  bool doPhotons(m_usePhotons || syst_type == PHSYST);
  bool doTaus(m_useTaus || syst_type == TAUSYST);

  for(const auto& systName: sysVec){

    // skip the empty case (we always run empty systematics in NOMINAL) except when in NOMINAL
    // e.g. if we do electron systematics, we've already handled the nominal case for electrons in NOMINAL
    if(systName.empty() && syst_type != NOMINAL) continue;

    // retrieve the systematic we're debugging
    //   do nothing for nominal (above switch case breaks if one of the below values isn't used so default not needed)
    switch ( static_cast<int>(syst_type) ) {
      case ELSYST:    ANA_CHECK(HelperFunctions::retrieve(inElectrons,  m_inContainerName_Electrons + systName, 0, m_store, msg())); break;
      case MUSYST:    ANA_CHECK(HelperFunctions::retrieve(inMuons,      m_inContainerName_Muons + systName,     0, m_store, msg())); break;
      case JETSYST:   ANA_CHECK(HelperFunctions::retrieve(inJets,       m_inContainerName_Jets + systName,      0, m_store, msg())); break;
      case PHSYST:    ANA_CHECK(HelperFunctions::retrieve(inPhotons,    m_inContainerName_Photons + systName,   0, m_store, msg())); break;
      case TAUSYST:   ANA_CHECK(HelperFunctions::retrieve(inTaus,       m_inContainerName_Taus + systName,      0, m_store, msg())); break;
    }

    // debug
    ANA_MSG_DEBUG("Container sizes before OR:");
    if(doElectrons) { ANA_MSG_DEBUG("\tinElectrons : " << inElectrons->size()); }
    if(doMuons)     { ANA_MSG_DEBUG("\tinMuons : " << inMuons->size()); }
    ANA_MSG_DEBUG("inJets : " << inJets->size() );
    if(doPhotons)   { ANA_MSG_DEBUG("\tinPhotons : " << inPhotons->size());  }
    if(doTaus)      { ANA_MSG_DEBUG("\tinTaus : " << inTaus->size());  }

    // do the actual OR
    ANA_MSG_DEBUG("Calling removeOverlaps()");
    ANA_CHECK( m_ORToolbox.masterTool->removeOverlaps(inElectrons, inMuons, inJets, inTaus, inPhotons));
    ANA_MSG_DEBUG("Done Calling removeOverlaps()");

    std::string ORdecor("passOR");
    if(m_useCutFlow){
      // fill cutflow histograms (always fill the object cutflow for that objects' systematics)
      ANA_MSG_DEBUG("Filling Cut Flow Histograms");
      if(doElectrons) fillObjectCutflow(inElectrons);
      if(doMuons)     fillObjectCutflow(inMuons);
      fillObjectCutflow(inJets);
      if(doPhotons)   fillObjectCutflow(inPhotons);
      if(doTaus)      fillObjectCutflow(inTaus);
    }

    // make a copy of input container(s) with selected objects
    if ( m_createSelectedContainers ) {
      // a different syst varied container will be created/stored for each syst variation
      ANA_MSG_DEBUG("Creating selected Containers");
      if(doElectrons) selectedElectrons  = new ConstDataVector<xAOD::ElectronContainer>(SG::VIEW_ELEMENTS);
      if(doMuons)     selectedMuons      = new ConstDataVector<xAOD::MuonContainer>(SG::VIEW_ELEMENTS);
      selectedJets       = new ConstDataVector<xAOD::JetContainer>(SG::VIEW_ELEMENTS);
      if(doPhotons)   selectedPhotons  = new ConstDataVector<xAOD::PhotonContainer>(SG::VIEW_ELEMENTS);
      if(doTaus)      selectedTaus = new ConstDataVector<xAOD::TauJetContainer>(SG::VIEW_ELEMENTS);

      ANA_MSG_DEBUG("Recording");
      if(doElectrons){ ANA_CHECK( m_store->record( selectedElectrons, m_outContainerName_Electrons + systName )); }
      if(doMuons)    { ANA_CHECK( m_store->record( selectedMuons, m_outContainerName_Muons + systName )); }
      ANA_CHECK(m_store->record( selectedJets,  m_outContainerName_Jets + systName ));
      if(doPhotons)  { ANA_CHECK( m_store->record( selectedPhotons, m_outContainerName_Photons + systName )); }
      if(doTaus)     { ANA_CHECK( m_store->record( selectedTaus, m_outContainerName_Taus + systName )); }
    }

    // resize containers basd on OR decision:
    //   if an object has been flagged as 'passOR', it will be stored in the 'selected' container
    ANA_MSG_DEBUG("Resizing");
    if(doElectrons){ ANA_CHECK( HelperFunctions::makeSubsetCont(inElectrons, selectedElectrons, msg(), ORdecor.c_str(), ToolName::SELECTOR)); }
    if(doMuons)    { ANA_CHECK( HelperFunctions::makeSubsetCont(inMuons, selectedMuons, msg(), ORdecor.c_str(), ToolName::SELECTOR)); }
    ANA_CHECK( HelperFunctions::makeSubsetCont(inJets, selectedJets, msg(), ORdecor.c_str(), ToolName::SELECTOR));
    if(doPhotons)  { ANA_CHECK( HelperFunctions::makeSubsetCont(inPhotons, selectedPhotons, msg(), ORdecor.c_str(), ToolName::SELECTOR)); }
    if(doTaus)     { ANA_CHECK( HelperFunctions::makeSubsetCont(inTaus, selectedTaus, msg(), ORdecor.c_str(), ToolName::SELECTOR)); }

    // debug
    ANA_MSG_DEBUG("Container sizes after OR");
    if(doElectrons){ ANA_MSG_DEBUG("\tselectedElectrons : " << selectedElectrons->size()); }
    if(doMuons)    { ANA_MSG_DEBUG("\tselectedMuons : " << selectedMuons->size()); }
    ANA_MSG_DEBUG("selectedJets : " << selectedJets->size());
    if(doPhotons)  { ANA_MSG_DEBUG("\tselectedPhotons : " << selectedPhotons->size()); }
    if(doTaus)     { ANA_MSG_DEBUG("\tselectedTaus : " << selectedTaus->size() ); }

    // for nominal, we push out the ""
    // for systematic, we push out the systematic name
    m_vecOutContainerNames->push_back(systName);
  } // end of loop over systematics

  return EL::StatusCode::SUCCESS;
}

EL::StatusCode OverlapRemover :: setCutFlowHist( )
{

  if ( m_useCutFlow ) {

    // retrieve the file in which the cutflow hists are stored
    //
    TFile *file     = wk()->getOutputFile ("cutflow");

    // retrieve the object cutflow
    //
    m_el_cutflowHist_1  = (TH1D*)file->Get("cutflow_electrons_1");
    m_el_cutflow_OR_cut   = m_el_cutflowHist_1->GetXaxis()->FindBin("OR_cut");
    m_mu_cutflowHist_1  = (TH1D*)file->Get("cutflow_muons_1");
    m_mu_cutflow_OR_cut   = m_mu_cutflowHist_1->GetXaxis()->FindBin("OR_cut");
    m_jet_cutflowHist_1   = (TH1D*)file->Get("cutflow_jets_1");
    m_jet_cutflow_OR_cut  = m_jet_cutflowHist_1->GetXaxis()->FindBin("OR_cut");
    m_ph_cutflowHist_1  = (TH1D*)file->Get("cutflow_photons_1");
    m_ph_cutflow_OR_cut   = m_ph_cutflowHist_1->GetXaxis()->FindBin("OR_cut");
    m_tau_cutflowHist_1   = (TH1D*)file->Get("cutflow_taus_1");
    m_tau_cutflow_OR_cut  = m_tau_cutflowHist_1->GetXaxis()->FindBin("OR_cut");
  }

  return EL::StatusCode::SUCCESS;
}

EL::StatusCode OverlapRemover :: setCounters( )
{
  m_numEvent      = 0;
  m_numObject     = 0;

  return EL::StatusCode::SUCCESS;
}

#include <EventLoop/Job.h>
#include <EventLoop/StatusCode.h>
#include <EventLoop/Worker.h>
#include "AthContainers/ConstDataVector.h"
#include "xAODEventInfo/EventInfo.h"
#include "xAODAnaHelpers/HelperFunctions.h"

#include <xAODAnaHelpers/MetHistsAlgo.h>

// this is needed to distribute the algorithm to the workers
ClassImp(MetHistsAlgo)

MetHistsAlgo :: MetHistsAlgo () :
    Algorithm("MetHistsAlgo")
{
}

EL::StatusCode MetHistsAlgo :: setupJob (EL::Job& job)
{
  job.useXAOD();

  // let's initialize the algorithm to use the xAODRootAccess package
  xAOD::Init("MetHistsAlgo").ignore(); // call before opening first file

  return EL::StatusCode::SUCCESS;
}

EL::StatusCode MetHistsAlgo :: histInitialize ()
{

  ANA_MSG_INFO( m_name );
  ANA_CHECK( xAH::Algorithm::algInitialize());
  if( m_inContainerName.empty() || m_detailStr.empty() ){
    ANA_MSG_ERROR( "One or more required configuration values are empty");
    return EL::StatusCode::FAILURE;
  }

  // declare class and add histograms to output
  m_plots = new MetHists(m_name, m_detailStr);
  ANA_CHECK( m_plots -> initialize());
  m_plots -> record( wk() );

  return EL::StatusCode::SUCCESS;
}

EL::StatusCode MetHistsAlgo :: fileExecute () { return EL::StatusCode::SUCCESS; }
EL::StatusCode MetHistsAlgo :: changeInput (bool /*firstFile*/) { return EL::StatusCode::SUCCESS; }

EL::StatusCode MetHistsAlgo :: initialize ()
{
  ANA_MSG_INFO( "MetHistsAlgo");
  m_event = wk()->xaodEvent();
  m_store = wk()->xaodStore();
  return EL::StatusCode::SUCCESS;
}

EL::StatusCode MetHistsAlgo :: execute ()
{
  const xAOD::EventInfo* eventInfo(nullptr);
  ANA_CHECK( HelperFunctions::retrieve(eventInfo, m_eventInfoContainerName, m_event, m_store, msg()) );


  float eventWeight(1);
  if( eventInfo->isAvailable< float >( "mcEventWeight" ) ) {
    eventWeight = eventInfo->auxdecor< float >( "mcEventWeight" );
  }

  const xAOD::MissingETContainer* met(nullptr);
  ANA_CHECK( HelperFunctions::retrieve(met, m_inContainerName, m_event, m_store, msg()) );

  ANA_CHECK( m_plots->execute( met, eventWeight ));

  return EL::StatusCode::SUCCESS;
}

EL::StatusCode MetHistsAlgo :: postExecute () { return EL::StatusCode::SUCCESS; }
EL::StatusCode MetHistsAlgo :: finalize () { return EL::StatusCode::SUCCESS; }
EL::StatusCode MetHistsAlgo :: histFinalize ()
{
  // clean up memory
  if(m_plots) delete m_plots;

  ANA_CHECK( xAH::Algorithm::algFinalize());
  return EL::StatusCode::SUCCESS;
}

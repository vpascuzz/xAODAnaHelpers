/**
 * @file   ToolCreator.h
 * @author Giordon Stark <gstark@cern.ch>
 * @brief  Algorithm that allows configuration and creation of tools through tool handles.
 *
 */

#ifndef xAODAnaHelpers_ToolCreator_H
#define xAODAnaHelpers_ToolCreator_H

// algorithm wrapper
#include "xAODAnaHelpers/Algorithm.h"

// external tools include(s):
#include "AsgTools/AnaToolHandle.h"
#include "AsgAnalysisInterfaces/IGoodRunsListSelectionTool.h"
#include "AsgAnalysisInterfaces/IPileupReweightingTool.h"
#include "TrigConfInterfaces/ITrigConfigTool.h"
#include "TrigDecisionTool/TrigDecisionTool.h"
#include "PATInterfaces/IWeightTool.h"

/**
  @rst
    This algorithm can be used to create and deep-configure tools using tool interfaces, and these tools can then be passed through to existing algorithms.

  @endrst
*/
class ToolCreator : public xAH::Algorithm
{
  public:
    std::string m_tool_type{""};
    std::string m_tool_name{""};
    std::string m_tool_configs{""};

  private:

    // tools
    asg::AnaToolHandle<T> m_tool_handle{""}; //!

  public:
    // this is a standard constructor
    ToolCreator ();

    // these are the functions inherited from Algorithm
    virtual EL::StatusCode setupJob (EL::Job& job);
    virtual EL::StatusCode fileExecute ();
    virtual EL::StatusCode histInitialize ();
    virtual EL::StatusCode changeInput (bool firstFile);
    virtual EL::StatusCode initialize ();
    virtual EL::StatusCode execute ();
    virtual EL::StatusCode postExecute ();
    virtual EL::StatusCode finalize ();
    virtual EL::StatusCode histFinalize ();

    /// @cond
    // this is needed to distribute the algorithm to the workers
    ClassDef(ToolCreator, 1);
    /// @endcond
};

#endif

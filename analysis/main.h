#ifndef main_h
#define main_h

#include "wwwtree.h"
#include "rooutil/rooutil.h"

#include "applicationregioncuts.h"
#include "basecuts.h"
#include "fakeratecontrolregioncuts.h"
#include "globalvariables.h"
#include "histograms.h"
#include "lostlepcontrolregioncuts.h"
#include "misccontrolregioncuts.h"
#include "promptcontrolregioncuts.h"
#include "scalefactors.h"
#include "signalregioncuts.h"
#include "systematiccuts.h"

bool passTrigger2016();
void bookCutflowsAndHistogramsForGivenRegions(RooUtil::Cutflow& cutflow, RooUtil::Histograms& histograms, TString regions);
void setGlobalConfigurationVariables(const char* input_paths, const char* output_file_name);
void setGlobalEventVariables();

#endif

// -*- C++ -*-
//
// Package:    CalibTracker/SiPixelESProducers
// Class:      SiPixelFakeGainForHLTReader
//
/**\class SiPixelFakeGainForHLTReader SiPixelFakeGainForHLTReader.cc SiPixelESProducers/test/SiPixelFakeGainForHLTReader.h

 Description: Test analyzer for fake pixel calibrationForHLT

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  Vincenzo CHIOCHIA
//         Created:  Tue Oct 17 17:40:56 CEST 2006
//
//

// system includes
#include <memory>

// user includes
#include "CalibTracker/SiPixelESProducers/interface/SiPixelGainCalibrationForHLTService.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/PluginManager/interface/ModuleDef.h"
#include "Geometry/CommonDetUnit/interface/PixelGeomDetUnit.h"
#include "Geometry/CommonTopologies/interface/PixelTopology.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"

// ROOT file
#include "TROOT.h"
#include "TFile.h"
#include "TTree.h"
#include "TBranch.h"
#include "TH1F.h"

namespace cms {
  class SiPixelFakeGainForHLTReader : public edm::one::EDAnalyzer<edm::one::WatchRuns> {
  public:
    explicit SiPixelFakeGainForHLTReader(const edm::ParameterSet& iConfig);
    ~SiPixelFakeGainForHLTReader() override = default;

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

    virtual void beginRun(const edm::Run&, const edm::EventSetup&) override;
    virtual void endRun(const edm::Run&, const edm::EventSetup&) override;
    virtual void analyze(const edm::Event&, const edm::EventSetup&) override;
    virtual void endJob() override;

  private:
    edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> trackerGeomToken_;
    edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> trackerGeomTokenBeginRun_;
    SiPixelGainCalibrationForHLTService SiPixelGainCalibrationForHLTService_;

    std::map<uint32_t, TH1F*> _TH1F_Pedestals_m;
    std::map<uint32_t, TH1F*> _TH1F_Gains_m;
    std::string filename_;
    TFile* fFile;
  };
}  // namespace cms

namespace cms {
  SiPixelFakeGainForHLTReader::SiPixelFakeGainForHLTReader(const edm::ParameterSet& conf)
      : trackerGeomToken_(esConsumes<TrackerGeometry, TrackerDigiGeometryRecord>()),
        trackerGeomTokenBeginRun_(esConsumes<TrackerGeometry, TrackerDigiGeometryRecord, edm::Transition::BeginRun>()),
        SiPixelGainCalibrationForHLTService_(conf, consumesCollector()),
        filename_(conf.getParameter<std::string>("fileName")) {}

  void SiPixelFakeGainForHLTReader::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
    unsigned int nmodules = 0;
    uint32_t nchannels = 0;
    fFile->cd();

    // Get the Geometry
    edm::ESHandle<TrackerGeometry> tkgeom = iSetup.getHandle(trackerGeomToken_);
    edm::LogInfo("SiPixelFakeGainForHLTReader") << " There are " << tkgeom->dets().size() << " detectors" << std::endl;

    //  for(TrackerGeometry::DetContainer::const_iterator it = tkgeom->dets().begin(); it != tkgeom->dets().end(); it++){
    //   if( dynamic_cast<PixelGeomDetUnit*>((*it))!=0){
    //     uint32_t detid=((*it)->geographicalId()).rawId();
    // Get the list of DetId's

    std::vector<uint32_t> vdetId_ = SiPixelGainCalibrationForHLTService_.getDetIds();
    // Loop over DetId's
    for (std::vector<uint32_t>::const_iterator detid_iter = vdetId_.begin(); detid_iter != vdetId_.end();
         detid_iter++) {
      uint32_t detid = *detid_iter;

      DetId detIdObject(detid);
      nmodules++;
      //if(nmodules>3) break;

      std::map<uint32_t, TH1F*>::iterator p_iter = _TH1F_Pedestals_m.find(detid);
      std::map<uint32_t, TH1F*>::iterator g_iter = _TH1F_Gains_m.find(detid);

      const GeomDetUnit* geoUnit = tkgeom->idToDetUnit(detIdObject);
      const PixelGeomDetUnit* pixDet = dynamic_cast<const PixelGeomDetUnit*>(geoUnit);
      const PixelTopology& topol = pixDet->specificTopology();

      // Get the module sizes.
      int nrows = topol.nrows();     // rows in x
      int ncols = topol.ncolumns();  // cols in y

      for (int col_iter = 0; col_iter < ncols; col_iter++) {
        for (int row_iter = 0; row_iter < nrows; row_iter++) {
          nchannels++;
          float ped = SiPixelGainCalibrationForHLTService_.getPedestal(detid, col_iter, row_iter);
          float gain = SiPixelGainCalibrationForHLTService_.getGain(detid, col_iter, row_iter);
          p_iter->second->Fill(ped);
          g_iter->second->Fill(gain);
        }
      }
    }

    edm::LogInfo("SiPixelFakeGainForHLTReader")
        << "[SiPixelFakeGainForHLTReader::analyze] ---> PIXEL Modules  " << nmodules << std::endl;
    edm::LogInfo("SiPixelFakeGainForHLTReader")
        << "[SiPixelFakeGainForHLTReader::analyze] ---> PIXEL Channels " << nchannels << std::endl;
    fFile->ls();
    fFile->Write();
    fFile->Close();
  }

  void SiPixelFakeGainForHLTReader::endRun(const edm::Run& run, const edm::EventSetup& iSetup) {}

  // ----------------------------------------------------------------------
  void SiPixelFakeGainForHLTReader::beginRun(const edm::Run& run, const edm::EventSetup& iSetup) {
    static int first(1);
    if (1 == first) {
      first = 0;
      edm::LogInfo("SiPixelFakeGainForHLTReader")
          << "[SiPixelFakeGainForHLTReader::beginJob] Opening ROOT file  " << std::endl;
      fFile = new TFile(filename_.c_str(), "RECREATE");
      fFile->mkdir("Pedestals");
      fFile->mkdir("Gains");
      fFile->cd();
      char name[128];

      // Get Geometry
      edm::ESHandle<TrackerGeometry> tkgeom = iSetup.getHandle(trackerGeomTokenBeginRun_);

      // Get the calibrationForHLT data
      SiPixelGainCalibrationForHLTService_.setESObjects(iSetup);
      edm::LogInfo("SiPixelFakeGainForHLTReader")
          << "[SiPixelFakeGainForHLTReader::beginJob] End Reading FakeGainForHLTects" << std::endl;
      // Get the list of DetId's
      std::vector<uint32_t> vdetId_ = SiPixelGainCalibrationForHLTService_.getDetIds();
      // Loop over DetId's
      for (std::vector<uint32_t>::const_iterator detid_iter = vdetId_.begin(); detid_iter != vdetId_.end();
           detid_iter++) {
        uint32_t detid = *detid_iter;

        const PixelGeomDetUnit* _PixelGeomDetUnit =
            dynamic_cast<const PixelGeomDetUnit*>(tkgeom->idToDetUnit(DetId(detid)));
        if (_PixelGeomDetUnit == 0) {
          edm::LogError("SiPixelFakeGainForHLTDisplay") << "[SiPixelFakeGainForHLTReader::beginJob] the detID " << detid
                                                        << " doesn't seem to belong to Tracker" << std::endl;
          continue;
        }
        // Book histograms
        sprintf(name, "Pedestals_%d", detid);
        fFile->cd();
        fFile->cd("Pedestals");
        _TH1F_Pedestals_m[detid] = new TH1F(name, name, 50, 0., 50.);
        sprintf(name, "Gains_%d", detid);
        fFile->cd();
        fFile->cd("Gains");
        _TH1F_Gains_m[detid] = new TH1F(name, name, 100, 0., 10.);
      }
    }
  }

  // ------------ method called once each job just after ending the event loop  ------------
  void SiPixelFakeGainForHLTReader::endJob() { std::cout << " ---> End job " << std::endl; }

  void SiPixelFakeGainForHLTReader::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;
    desc.add<std::string>("fileName", "out.root");
    descriptions.addWithDefaultLabel(desc);
  }
}  // namespace cms

using cms::SiPixelFakeGainForHLTReader;
DEFINE_FWK_MODULE(SiPixelFakeGainForHLTReader);

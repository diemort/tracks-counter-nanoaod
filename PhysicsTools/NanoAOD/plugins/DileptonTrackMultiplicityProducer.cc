#include "FWCore/Framework/interface/global/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "DataFormats/NanoAOD/interface/FlatTable.h"

#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/PatCandidates/interface/Electron.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/VertexReco/interface/Vertex.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include <cmath>
#include <memory>
#include <string>
#include <vector>

class DileptonTrackMultiplicityProducer : public edm::global::EDProducer<> {
public:
  explicit DileptonTrackMultiplicityProducer(const edm::ParameterSet&);
  ~DileptonTrackMultiplicityProducer() override = default;

  void produce(edm::StreamID, edm::Event&, const edm::EventSetup&) const override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  edm::EDGetTokenT<std::vector<pat::Muon>> muonsToken_;
  edm::EDGetTokenT<std::vector<pat::Electron>> electronsToken_;
  edm::EDGetTokenT<std::vector<pat::PackedCandidate>> tracksToken_;
  edm::EDGetTokenT<std::vector<reco::Vertex>> verticesToken_;

  const std::string leptonType_;
  const double maxDz_;
  const double minPt_;
  const bool requireHighPurity_;
  const double maxDxy_;
  const int verbose_;
};

DileptonTrackMultiplicityProducer::DileptonTrackMultiplicityProducer(
    const edm::ParameterSet& cfg)
    : muonsToken_(consumes(cfg.getParameter<edm::InputTag>("muons"))),
      electronsToken_(consumes(cfg.getParameter<edm::InputTag>("electrons"))),
      tracksToken_(consumes(cfg.getParameter<edm::InputTag>("tracks"))),
      verticesToken_(consumes(cfg.getParameter<edm::InputTag>("vertices"))),
      leptonType_(cfg.getParameter<std::string>("leptonType")),
      maxDz_(cfg.getParameter<double>("maxDz")),
      minPt_(cfg.getParameter<double>("minPt")),
      requireHighPurity_(cfg.getParameter<bool>("requireHighPurity")),
      maxDxy_(cfg.getParameter<double>("maxDxy")),
      verbose_(cfg.getUntrackedParameter<int>("verbose", 0)) {

  produces<nanoaod::FlatTable>("DileptonTrk");
}

void DileptonTrackMultiplicityProducer::produce(
    edm::StreamID,
    edm::Event& event,
    const edm::EventSetup&) const {

  if (verbose_ >= 1) {
    edm::LogPrint("DileptonTrackMultiplicity")
        << "[DileptonTrackMultiplicity] produce() called for event "
        << event.id();
  }

  // --- Primary vertex fallback
  edm::Handle<std::vector<reco::Vertex>> vertices;
  event.getByToken(verticesToken_, vertices);

  double zRef = 0.0;
  if (vertices.isValid() && !vertices->empty()) {
    zRef = vertices->front().z();
  }

  // --- Dilepton vertex proxy
  if (leptonType_ == "muon") {
    edm::Handle<std::vector<pat::Muon>> muons;
    event.getByToken(muonsToken_, muons);
    if (muons.isValid() && muons->size() == 2) {
      zRef = 0.5 * (muons->at(0).vz() + muons->at(1).vz());
    }
  } else if (leptonType_ == "electron") {
    edm::Handle<std::vector<pat::Electron>> electrons;
    event.getByToken(electronsToken_, electrons);
    if (electrons.isValid() && electrons->size() == 2) {
      zRef = 0.5 * (electrons->at(0).vz() + electrons->at(1).vz());
    }
  }

  edm::Handle<std::vector<pat::PackedCandidate>> tracks;
  event.getByToken(tracksToken_, tracks);

  int nTracks = 0;

  if (tracks.isValid()) {
    for (const auto& cand : *tracks) {
      if (cand.charge() == 0) continue;
      if (cand.pt() < minPt_) continue;

      if (requireHighPurity_) {
        if (!cand.hasTrackDetails()) continue;
        if (!cand.trackHighPurity()) continue;
      }

      const double dzToDiLep = std::abs(cand.vz() - zRef);
      if (dzToDiLep > maxDz_) continue;

      if (maxDxy_ > 0.0 && std::abs(cand.dxy()) > maxDxy_) continue;

      ++nTracks;
    }
  }

  if (verbose_ >= 1) {
    edm::LogPrint("DileptonTrackMultiplicity")
        << "Run " << event.id().run()
        << " Lumi " << event.id().luminosityBlock()
        << " Event " << event.id().event()
        << " | zRef = " << zRef
        << " | nTracksPV = " << nTracks;
  }

  auto table = std::make_unique<nanoaod::FlatTable>(
      1,                // one row
      "DileptonTrk",    // table name
      true,             // singleton
      false             // not extension
  );

  table->addColumn<int>(
      "nTracksPV",
      std::vector<int>{nTracks},
      "Number of charged tracks near dilepton vertex"
  );

  event.put(std::move(table), "DileptonTrk");
}

void DileptonTrackMultiplicityProducer::fillDescriptions(
    edm::ConfigurationDescriptions& descriptions) {

  edm::ParameterSetDescription desc;

  desc.add<std::string>("leptonType", "muon");
  desc.add<edm::InputTag>("muons", edm::InputTag("slimmedMuons"));
  desc.add<edm::InputTag>("electrons", edm::InputTag("slimmedElectrons"));
  desc.add<edm::InputTag>("tracks", edm::InputTag("packedPFCandidates"));
  desc.add<edm::InputTag>("vertices", edm::InputTag("offlineSlimmedPrimaryVertices"));

  desc.add<double>("maxDz", 0.5);
  desc.add<double>("minPt", 0.4);
  desc.add<bool>("requireHighPurity", true);
  desc.add<double>("maxDxy", 0.2);
  desc.addUntracked<int>("verbose", 0);

  descriptions.add("dileptonTrackMultiplicity", desc);
}

DEFINE_FWK_MODULE(DileptonTrackMultiplicityProducer);

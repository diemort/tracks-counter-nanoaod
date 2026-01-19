#include "FWCore/Framework/interface/global/EDProducer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"

#include "DataFormats/NanoAOD/interface/FlatTable.h"

#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/PatCandidates/interface/Electron.h"
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
#include "DataFormats/VertexReco/interface/Vertex.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include <vector>
#include <cmath>

class DileptonTrackMultiplicityProducer : public edm::global::EDProducer<> {
public:
  explicit DileptonTrackMultiplicityProducer(const edm::ParameterSet&);
  ~DileptonTrackMultiplicityProducer() override = default;

  void produce(edm::StreamID, edm::Event&, const edm::EventSetup&) const override;
  static void fillDescriptions(edm::ConfigurationDescriptions&);

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
  const int minLayers_;
  const double maxChi2_;
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
      minLayers_(cfg.getParameter<int>("minTrackerLayers")),
      maxChi2_(cfg.getParameter<double>("maxChi2")),
      verbose_(cfg.getUntrackedParameter<int>("verbose", 0)) {

  produces<nanoaod::FlatTable>("DileptonTrk");
  produces<nanoaod::FlatTable>("DileptonTrkTrack");
}

void DileptonTrackMultiplicityProducer::produce(
    edm::StreamID,
    edm::Event& event,
    const edm::EventSetup&) const {

  // -------------------------------------------------------
  // Reference vertex (dilepton or PV fallback)
  // -------------------------------------------------------
  edm::Handle<std::vector<reco::Vertex>> vertices;
  event.getByToken(verticesToken_, vertices);

  double zRef = 0.0;
  if (vertices.isValid() && !vertices->empty())
    zRef = vertices->front().z();

  if (leptonType_ == "muon") {
    edm::Handle<std::vector<pat::Muon>> muons;
    event.getByToken(muonsToken_, muons);
    if (muons.isValid() && muons->size() == 2)
      zRef = 0.5 * (muons->at(0).vz() + muons->at(1).vz());
  }

  if (leptonType_ == "electron") {
    edm::Handle<std::vector<pat::Electron>> electrons;
    event.getByToken(electronsToken_, electrons);
    if (electrons.isValid() && electrons->size() == 2)
      zRef = 0.5 * (electrons->at(0).vz() + electrons->at(1).vz());
  }

  // -------------------------------------------------------
  // Track loop
  // -------------------------------------------------------
  edm::Handle<std::vector<pat::PackedCandidate>> tracks;
  event.getByToken(tracksToken_, tracks);

  std::vector<float> v_pt, v_eta, v_phi, v_dz, v_dxy;
  int nTracks = 0;

  if (tracks.isValid()) {
    for (const auto& cand : *tracks) {

      if (cand.charge() == 0) continue;
      if (cand.pt() < minPt_) continue;
      if (!cand.hasTrackDetails()) continue;

      const auto& trk = cand.pseudoTrack();

      if (requireHighPurity_ && !cand.trackHighPurity()) continue;
      if (trk.hitPattern().trackerLayersWithMeasurement() < minLayers_) continue;
      if (trk.normalizedChi2() > maxChi2_) continue;
      if (std::abs(cand.vz() - zRef) > maxDz_) continue;
      if (maxDxy_ > 0 && std::abs(cand.dxy()) > maxDxy_) continue;

      v_pt.push_back(cand.pt());
      v_eta.push_back(cand.eta());
      v_phi.push_back(cand.phi());
      v_dz.push_back(cand.vz() - zRef);
      v_dxy.push_back(cand.dxy());

      ++nTracks;
    }
  }

  // -------------------------------------------------------
  // Event-level table
  // -------------------------------------------------------
  auto evtTable = std::make_unique<nanoaod::FlatTable>(
      1, "DileptonTrk", true, false);

  std::vector<int> nTracksVec(1, nTracks);

  evtTable->addColumn<int>(
      "nTracksPV",
      nTracksVec,
      "Number of charged tracks near dilepton vertex"
  );

  event.put(std::move(evtTable), "DileptonTrk");

  // -------------------------------------------------------
  // Track collection table
  // -------------------------------------------------------
  auto trkTable = std::make_unique<nanoaod::FlatTable>(
      v_pt.size(), "DileptonTrkTrack", false, false);

  trkTable->addColumn<float>("pt", v_pt, "Track pT");
  trkTable->addColumn<float>("eta", v_eta, "Track eta");
  trkTable->addColumn<float>("phi", v_phi, "Track phi");
  trkTable->addColumn<float>("dz", v_dz, "dz wrt dilepton vertex");
  trkTable->addColumn<float>("dxy", v_dxy, "dxy wrt beamspot");

  event.put(std::move(trkTable), "DileptonTrkTrack");

  if (verbose_ >= 1) {
    edm::LogPrint("DileptonTrackMultiplicity")
        << "Event " << event.id()
        << " zRef=" << zRef
        << " nTracksPV=" << nTracks;
  }
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
  desc.add<int>("minTrackerLayers", 6);
  desc.add<double>("maxChi2", 5.0);
  desc.addUntracked<int>("verbose", 0);

  descriptions.add("dileptonTrackMultiplicity", desc);
}

DEFINE_FWK_MODULE(DileptonTrackMultiplicityProducer);


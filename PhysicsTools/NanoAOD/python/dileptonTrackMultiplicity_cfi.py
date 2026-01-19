import FWCore.ParameterSet.Config as cms

dileptonTrackMultiplicity = cms.EDProducer(
    "DileptonTrackMultiplicityProducer",

    leptonType = cms.string("muon"),  # "muon" or "electron"

    muons     = cms.InputTag("slimmedMuons"),
    electrons = cms.InputTag("slimmedElectrons"),
    tracks    = cms.InputTag("packedPFCandidates"),
    vertices  = cms.InputTag("offlineSlimmedPrimaryVertices"),

    maxDz = cms.double(0.5),  # cm
    minPt = cms.double(0.4),

    # --- verbosity control (0 = silent, >=1 = log output)
    verbose = cms.untracked.int32(0),
)

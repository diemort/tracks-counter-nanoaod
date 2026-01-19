import FWCore.ParameterSet.Config as cms

dileptonTrackMultiplicity = cms.EDProducer(
    "DileptonTrackMultiplicityProducer",

    leptonType = cms.string("muon"),

    muons     = cms.InputTag("slimmedMuons"),
    electrons = cms.InputTag("slimmedElectrons"),
    tracks    = cms.InputTag("packedPFCandidates"),
    vertices  = cms.InputTag("offlineSlimmedPrimaryVertices"),

    minPt = cms.double(0.4),
    maxDz = cms.double(0.5),
    maxDxy = cms.double(0.2),

    requireHighPurity = cms.bool(True),
    minTrackerLayers = cms.int32(6),
    maxChi2 = cms.double(5.0),

    verbose = cms.untracked.int32(0)
)

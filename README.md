# Dilepton Track Multiplicity (NanoAOD)

## Overview

This development adds an **event-level track multiplicity observable**
associated with a **dilepton vertex**, intended primarily for:

- exclusivity studies (e.g. γγ > l+l-),
- pileup diagnostics,
- PPS / forward physics analyses,
- validation of clean dilepton topologies.

The implementation is based on a **custom C++ EDProducer** operating at the
**MiniAOD level**, producing a `nanoaod::FlatTable` that is written directly
into the NanoAOD `Events` tree.

---

## Physics Definition

The observable:

```
nTracksPV
```

is defined as the number of **charged PF candidates** compatible with the
**dilepton vertex**, where the dilepton vertex is approximated by:

- the **average z-position** of the two leptons (muons or electrons), or
- the **primary vertex z** as fallback if exactly two leptons are not present.

This choice is robust, MiniAOD-compatible, and standard in CMS exclusive
analyses.

---

## Track Selection Criteria

A PF candidate is counted if **all** of the following are satisfied:

1. **Charged particle**
   ```
   cand.charge() != 0
   ```

2. **Minimum transverse momentum**
   ```
   cand.pt() > minPt   (default: 0.4 GeV)
   ```

3. **High-purity track (optional, enabled by default)**
   ```
   cand.hasTrackDetails()
   cand.trackHighPurity()
   ```

4. **Longitudinal compatibility with dilepton vertex**
   ```
   |cand.vz − z_dilepton| < maxDz   (default: 0.5 cm)
   ```

5. **Transverse impact parameter cut (optional)**
   ```
   |cand.dxy| < maxDxy   (default: 0.2 cm)
   ```

⚠️ By construction, the lepton tracks themselves are included in the count.
If only *additional tracks* are desired, leptons must be explicitly excluded.

---

## Implementation Details

### C++ Producer

- Class: `DileptonTrackMultiplicityProducer`
- Type: `edm::global::EDProducer<>`
- Output: `nanoaod::FlatTable`
- Table name: **`DileptonTrk`**
- Singleton: **true**
- Extension: **false**

```cpp
auto table = std::make_unique<nanoaod::FlatTable>(
    1,
    "DileptonTrk",
    false,   // NOT an extension table
    true     // singleton (one row per event)
);
```

Event-level quantities **must** be written as a non-extension singleton table.
Failure to do so will trigger:

```
Trying to save an extension table before having saved the corresponding main table
```

---

## Configuration (CMSSW)

Example configuration snippet:

```python
process.load("PhysicsTools.NanoAOD.dileptonTrackMultiplicity_cfi")

process.nanoTableTaskFS.add(process.dileptonTrackMultiplicity)

process.nanoAOD_step = cms.Path(
    process.nanoSequenceMC *
    process.dileptonTrackMultiplicity
)
```

The table is written automatically by the `NanoAODOutputModule` when using:

```python
process.NANOAODSIMoutput.outputCommands.append(
    "keep nanoaodFlatTable_*_*_*"
)
```

---

## Output in NanoAOD

The following branch appears in the `Events` tree:

```
DileptonTrk_nTracksPV
```

Example ROOT usage:

```cpp
Events->Scan("DileptonTrk_nTracksPV");
```

or in Python:

```python
df = ROOT.RDataFrame("Events", "test.root")
df.Histo1D("DileptonTrk_nTracksPV").Draw()
```

---

## Logging and Debugging

The producer emits detailed runtime logs via:

```cpp
edm::LogPrint("DileptonTrackMultiplicity")
```

To enable full verbosity:

```python
process.MessageLogger.cerr.enable = True
process.MessageLogger.cerr.DileptonTrackMultiplicity = cms.untracked.PSet(
    limit = cms.untracked.int32(1000000)
)
```

---

## Known Pitfalls (Lessons Learned)

- Do not write event-level quantities as extension tables.
- `edmDumpEventContent` is not reliable for NanoAOD FlatTables.
- `SimpleFlatTableProducer` is not appropriate for computed scalars.
- Always use `FlatTable(singleton=true, extension=false)` for event variables.
- Always add the producer to `nanoTableTaskFS`.

---

## Possible Extensions

- Exclude lepton tracks from the count
- Split counts by pT threshold
- Add pileup-sensitive sidebands
- Store `zRef` itself for validation
- Add ΔR-based lepton veto

---

## Notes

Developed and validated primarily in **CMSSW_15_0_17**, with cross-checks in
**CMSSW_12_5_X**. Designed for exclusive dilepton + PPS analyses.

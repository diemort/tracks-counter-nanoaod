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

## Track Kinematics Stored in NanoAOD

In addition to the event-level multiplicity, the producer can be extended
to store **per-track kinematic and quality information** in a separate
FlatTable (non-singleton):

- `Track_pt`  — transverse momentum
- `Track_eta` — pseudorapidity
- `Track_phi` — azimuthal angle
- `Track_dz`  — longitudinal distance to dilepton vertex
- `Track_dxy` — transverse impact parameter
- `Track_layers` — tracker layers with measurement
- `Track_chi2` — normalized χ²
- `Track_highPurity` — track quality flag

These variables allow detailed studies of pileup structure and exclusivity.

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

## Verbosity and Logging Control

The producer supports a runtime verbosity flag:

```python
verbose = cms.untracked.int32(0)
```

- `0` (default): silent mode (production)
- `1`: per-event diagnostic output

When enabled, the producer logs:

- run / lumi / event identifiers
- dilepton reference vertex (`zRef`)
- computed track multiplicity (`nTracksPV`)

To activate logging:

```python
process.dileptonTrackMultiplicity.verbose = 1

process.MessageLogger.cerr.enable = True
process.MessageLogger.cerr.DileptonTrackMultiplicity = cms.untracked.PSet(
    limit = cms.untracked.int32(1000000)
)
```

---

## Known Pitfalls

- Do not write event-level quantities as extension tables.
- `edmDumpEventContent` is not reliable for NanoAOD FlatTables.
- `SimpleFlatTableProducer` is not appropriate for computed scalars.
- Always use `FlatTable(singleton=true, extension=false)` for event variables.
- Always add the producer to `nanoTableTaskFS`.

---

## Notes

Developed and validated primarily in **CMSSW_12_5_0**.
Designed for exclusive dilepton + PPS analyses.

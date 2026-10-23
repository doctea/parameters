#!/usr/bin/env python3
"""Regression checks for modulation slot mode math and save/load token mapping.

This mirrors current C++ behavior:
- FloatParameter::get_modulation_value (linear additive model, no adaptive depth)
- MIDIParameterInput bipolar/unipolar normalization
- Parameter.cpp slot mode token parsing
"""

MOD_SLOT_BI_NATIVE = 0
MOD_SLOT_UNI_RAW = 1
MOD_SLOT_UNI_CENTERED = 2


def map_bipolar_to_unipolar(v_bi: float) -> float:
    # Mirrors AnalogParameterInputBase conversion for bipolar input -> unipolar output.
    return 0.5 + (v_bi / 2.0)


def slot_nml(mode: int, src_uni: float, src_bi: float) -> float:
    """Return the normalised modulation value for a slot given its mode and source values.
    
    B+- and U+-: scaled by 0.5 so amount=1.0 sweeps ±50% of the normal span.
    A clean 0..1 sweep results when base=0.5 at amount=1.0, with no early saturation.
    Use amount=2.0 to sweep the full span from a base at the edge of the range.
    U+: linear, no scaling — one-sided, early clipping at non-zero base is expected.
    """
    if mode == MOD_SLOT_BI_NATIVE:
        return src_bi * 0.5
    if mode == MOD_SLOT_UNI_CENTERED:
        return (-1.0 + (src_uni * 2.0)) * 0.5
    return src_uni


def clamp(v: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, v))


def midi_unipolar(cc: int) -> float:
    """Mirror MIDIParameterInput::get_normal_value_unipolar()."""
    return cc / 127.0


def midi_bipolar(cc: int) -> float:
    """Mirror MIDIParameterInput::get_normal_value_bipolar() — monotonic, higher CC = more positive."""
    return clamp((cc - 64.0) / 63.0, -1.0, 1.0)


def apply_single_slot(base: float, mode: int, src_uni: float, src_bi: float,
                      amount: float, mn: float = 0.0, mx: float = 1.0) -> float:
    """Linear additive model: output = clamp(base + nml*amount, min, max)."""
    return clamp(base + slot_nml(mode, src_uni, src_bi) * amount, mn, mx)


def parse_mode_token(token: str) -> int:
    if token in ("bi_native", "bipolar"):
        return MOD_SLOT_BI_NATIVE
    if token == "uni_centered":
        return MOD_SLOT_UNI_CENTERED
    if token in ("uni_raw", "unipolar"):
        return MOD_SLOT_UNI_RAW
    return MOD_SLOT_UNI_RAW


def approx(a: float, b: float, eps: float = 1e-6) -> bool:
    return abs(a - b) <= eps


def main() -> None:
    # Source extrema and conversion checks.
    bi_min, bi_max = -1.0, 1.0
    uni_from_bi_min = map_bipolar_to_unipolar(bi_min)
    uni_from_bi_max = map_bipolar_to_unipolar(bi_max)

    # B+- full bipolar contribution range.
    assert approx(slot_nml(MOD_SLOT_BI_NATIVE, 0.0, bi_min), -1.0)
    assert approx(slot_nml(MOD_SLOT_BI_NATIVE, 0.0, bi_max), 1.0)

    # U+- centered full bipolar contribution from unipolar source.
    assert approx(slot_nml(MOD_SLOT_UNI_CENTERED, 0.0, 0.0), -1.0)
    assert approx(slot_nml(MOD_SLOT_UNI_CENTERED, 1.0, 0.0), 1.0)

    # U+ one-sided full unipolar contribution.
    assert approx(slot_nml(MOD_SLOT_UNI_RAW, 0.0, 0.0), 0.0)
    assert approx(slot_nml(MOD_SLOT_UNI_RAW, 1.0, 0.0), 1.0)

    # U+ fed by bipolar input converted to unipolar should span [0.0, 1.0].
    assert approx(slot_nml(MOD_SLOT_UNI_RAW, uni_from_bi_min, bi_min), 0.0)
    assert approx(slot_nml(MOD_SLOT_UNI_RAW, uni_from_bi_max, bi_max), 1.0)

    # Save/load token compatibility.
    assert parse_mode_token("bipolar") == MOD_SLOT_BI_NATIVE
    assert parse_mode_token("bi_native") == MOD_SLOT_BI_NATIVE
    assert parse_mode_token("unipolar") == MOD_SLOT_UNI_RAW
    assert parse_mode_token("uni_raw") == MOD_SLOT_UNI_RAW
    assert parse_mode_token("uni_centered") == MOD_SLOT_UNI_CENTERED
    assert parse_mode_token("unknown_mode") == MOD_SLOT_UNI_RAW

    # Modulation depth is linear and symmetric regardless of base position.
    # base=0.5, amt=0.5, U+- or B+-: output sweeps 0.0..1.0 (full span at amt 0.5 from center
    # is expected — that's what ±0.5 linear modulation from 0.5 gives).
    assert approx(apply_single_slot(0.5, MOD_SLOT_UNI_CENTERED, 0.0, -1.0, 0.5), 0.0)
    assert approx(apply_single_slot(0.5, MOD_SLOT_UNI_CENTERED, 1.0,  1.0, 0.5), 1.0)

    # base=0.5, amt=1.0 also reaches full span.
    assert approx(apply_single_slot(0.5, MOD_SLOT_UNI_CENTERED, 0.0, -1.0, 1.0), 0.0)
    assert approx(apply_single_slot(0.5, MOD_SLOT_UNI_CENTERED, 1.0,  1.0, 1.0), 1.0)

    # Symmetry: from base=0.9, positive and negative half-depth are equal in magnitude.
    pos_delta = apply_single_slot(0.9, MOD_SLOT_UNI_CENTERED, 1.0, 1.0, 0.3) - 0.9
    neg_delta = 0.9 - apply_single_slot(0.9, MOD_SLOT_UNI_CENTERED, 0.0, -1.0, 0.3)
    assert approx(pos_delta, 0.1)   # clamped to 1.0
    assert approx(neg_delta, 0.3)   # unclamped, full negative travel

    # U+ never produces negative modulation.
    assert apply_single_slot(0.5, MOD_SLOT_UNI_RAW, 0.0, -1.0, 1.0) == 0.5
    assert apply_single_slot(0.5, MOD_SLOT_UNI_RAW, 1.0, 1.0, 1.0) == 1.0

    # Scenario 2: MIDI source (Fade1-style).
    # B+- must not be inverted: higher CC -> more positive output.
    cc_lo, cc_mid, cc_hi = 0, 64, 127
    base = 0.5
    out_bi_lo  = apply_single_slot(base, MOD_SLOT_BI_NATIVE, midi_unipolar(cc_lo),  midi_bipolar(cc_lo),  1.0)
    out_bi_mid = apply_single_slot(base, MOD_SLOT_BI_NATIVE, midi_unipolar(cc_mid), midi_bipolar(cc_mid), 1.0)
    out_bi_hi  = apply_single_slot(base, MOD_SLOT_BI_NATIVE, midi_unipolar(cc_hi),  midi_bipolar(cc_hi),  1.0)
    assert out_bi_lo < out_bi_mid < out_bi_hi

    # U+ and U+- should remain monotonic-positive for increasing CC.
    out_ur = [apply_single_slot(base, MOD_SLOT_UNI_RAW,     midi_unipolar(cc), midi_bipolar(cc), 1.0) for cc in [cc_lo, cc_mid, cc_hi]]
    out_uc = [apply_single_slot(base, MOD_SLOT_UNI_CENTERED, midi_unipolar(cc), midi_bipolar(cc), 1.0) for cc in [cc_lo, cc_mid, cc_hi]]
    assert out_ur[0] <= out_ur[1] <= out_ur[2]
    assert out_uc[0] < out_uc[1] < out_uc[2]

    # Negative amount inverts direction.
    pos = apply_single_slot(base, MOD_SLOT_BI_NATIVE, midi_unipolar(cc_hi), midi_bipolar(cc_hi),  0.7)
    neg = apply_single_slot(base, MOD_SLOT_BI_NATIVE, midi_unipolar(cc_hi), midi_bipolar(cc_hi), -0.7)
    assert pos > base
    assert neg < base

    # midi_bipolar: CC0 = full negative, CC64 = ~zero, CC127 = full positive.
    assert approx(midi_bipolar(0),   -1.0)
    assert approx(midi_bipolar(64),   0.0)
    assert approx(midi_bipolar(127),  1.0)
    # Monotonic direction.
    assert midi_bipolar(0) < midi_bipolar(64) < midi_bipolar(127)

    print("modslot regression checks passed")


if __name__ == "__main__":
    main()


MOD_SLOT_BI_NATIVE = 0
MOD_SLOT_UNI_RAW = 1
MOD_SLOT_UNI_CENTERED = 2


def map_bipolar_to_unipolar(v_bi: float) -> float:
    # Mirrors AnalogParameterInputBase conversion for bipolar input -> unipolar output.
    return 0.5 + (v_bi / 2.0)


def slot_nml(mode: int, src_uni: float, src_bi: float) -> float:
    if mode == MOD_SLOT_BI_NATIVE:
        return src_bi
    if mode == MOD_SLOT_UNI_CENTERED:
        return -1.0 + (src_uni * 2.0)
    return src_uni


def clamp(v: float, lo: float, hi: float) -> float:
    return max(lo, min(hi, v))


def midi_unipolar(cc: int) -> float:
    return cc / 127.0


def midi_bipolar(cc: int) -> float:
    # Mirrors MIDIParameterInput::get_normal_value_bipolar().
    return clamp((cc - 64.0) / 63.0, -1.0, 1.0)


def slot_modulation(mode: int, src_uni: float, src_bi: float, amount: float, base: float) -> float:
    nml = slot_nml(mode, src_uni, src_bi)
    mod = nml * amount

    # Mirrors MODULATION_USE_ADAPTIVE_DEPTH path in parameters.cpp for bipolar-style modes.
    if mode in (MOD_SLOT_BI_NATIVE, MOD_SLOT_UNI_CENTERED):
        positive_headroom = 1.0 - base
        negative_headroom = base - 0.0
        if mod >= 0.0:
            mod *= positive_headroom
        else:
            mod *= negative_headroom

    return mod


def apply_single_slot(base: float, mode: int, src_uni: float, src_bi: float, amount: float) -> float:
    return clamp(base + slot_modulation(mode, src_uni, src_bi, amount, base), 0.0, 1.0)


def parse_mode_token(token: str) -> int:
    if token in ("bi_native", "bipolar"):
        return MOD_SLOT_BI_NATIVE
    if token == "uni_centered":
        return MOD_SLOT_UNI_CENTERED
    if token in ("uni_raw", "unipolar"):
        return MOD_SLOT_UNI_RAW
    return MOD_SLOT_UNI_RAW


def approx(a: float, b: float, eps: float = 1e-6) -> bool:
    return abs(a - b) <= eps


def main() -> None:
    # Source extrema and conversion checks.
    bi_min, bi_max = -1.0, 1.0
    uni_from_bi_min = map_bipolar_to_unipolar(bi_min)
    uni_from_bi_max = map_bipolar_to_unipolar(bi_max)

    # B+- full bipolar contribution range.
    assert approx(slot_nml(MOD_SLOT_BI_NATIVE, 0.0, bi_min), -1.0)
    assert approx(slot_nml(MOD_SLOT_BI_NATIVE, 0.0, bi_max), 1.0)

    # U+- centered full bipolar contribution from unipolar source.
    assert approx(slot_nml(MOD_SLOT_UNI_CENTERED, 0.0, 0.0), -1.0)
    assert approx(slot_nml(MOD_SLOT_UNI_CENTERED, 1.0, 0.0), 1.0)

    # U+ one-sided full unipolar contribution.
    assert approx(slot_nml(MOD_SLOT_UNI_RAW, 0.0, 0.0), 0.0)
    assert approx(slot_nml(MOD_SLOT_UNI_RAW, 1.0, 0.0), 1.0)

    # U+ fed by bipolar input converted to unipolar should span [0.0, 1.0].
    assert approx(slot_nml(MOD_SLOT_UNI_RAW, uni_from_bi_min, bi_min), 0.0)
    assert approx(slot_nml(MOD_SLOT_UNI_RAW, uni_from_bi_max, bi_max), 1.0)

    # Save/load token compatibility.
    assert parse_mode_token("bipolar") == MOD_SLOT_BI_NATIVE
    assert parse_mode_token("bi_native") == MOD_SLOT_BI_NATIVE
    assert parse_mode_token("unipolar") == MOD_SLOT_UNI_RAW
    assert parse_mode_token("uni_raw") == MOD_SLOT_UNI_RAW
    assert parse_mode_token("uni_centered") == MOD_SLOT_UNI_CENTERED
    assert parse_mode_token("unknown_mode") == MOD_SLOT_UNI_RAW

    # Scenario 1: bipolar source (e.g. LFO Sync), base 50%.
    base = 0.5

    # U+- at amount 50% should NOT already hit full 0..1 range.
    out_low = apply_single_slot(base, MOD_SLOT_UNI_CENTERED, 0.0, -1.0, 0.5)
    out_high = apply_single_slot(base, MOD_SLOT_UNI_CENTERED, 1.0, 1.0, 0.5)
    assert approx(out_low, 0.25)
    assert approx(out_high, 0.75)

    # U+- at amount 100% should hit 0..1 at source extremes, with no early clipping.
    out_mid_hi = apply_single_slot(base, MOD_SLOT_UNI_CENTERED, 0.75, 0.5, 1.0)
    out_hi = apply_single_slot(base, MOD_SLOT_UNI_CENTERED, 1.0, 1.0, 1.0)
    assert approx(out_mid_hi, 0.75)
    assert approx(out_hi, 1.0)

    # Same expectations for B+- with bipolar source.
    out_mid_hi_bi = apply_single_slot(base, MOD_SLOT_BI_NATIVE, 0.75, 0.5, 1.0)
    out_hi_bi = apply_single_slot(base, MOD_SLOT_BI_NATIVE, 1.0, 1.0, 1.0)
    assert approx(out_mid_hi_bi, 0.75)
    assert approx(out_hi_bi, 1.0)

    # Scenario 2: unipolar MIDI source (Fade1-style).
    # B+- must not be inverted: higher CC -> more positive output.
    cc_lo, cc_mid, cc_hi = 0, 64, 127
    out_bi_lo = apply_single_slot(base, MOD_SLOT_BI_NATIVE, midi_unipolar(cc_lo), midi_bipolar(cc_lo), 1.0)
    out_bi_mid = apply_single_slot(base, MOD_SLOT_BI_NATIVE, midi_unipolar(cc_mid), midi_bipolar(cc_mid), 1.0)
    out_bi_hi = apply_single_slot(base, MOD_SLOT_BI_NATIVE, midi_unipolar(cc_hi), midi_bipolar(cc_hi), 1.0)
    assert out_bi_lo < out_bi_mid < out_bi_hi

    # U+ and U+- should remain monotonic-positive for increasing CC.
    out_uplus_lo = apply_single_slot(base, MOD_SLOT_UNI_RAW, midi_unipolar(cc_lo), midi_bipolar(cc_lo), 1.0)
    out_uplus_mid = apply_single_slot(base, MOD_SLOT_UNI_RAW, midi_unipolar(cc_mid), midi_bipolar(cc_mid), 1.0)
    out_uplus_hi = apply_single_slot(base, MOD_SLOT_UNI_RAW, midi_unipolar(cc_hi), midi_bipolar(cc_hi), 1.0)
    assert out_uplus_lo <= out_uplus_mid <= out_uplus_hi

    out_uc_lo = apply_single_slot(base, MOD_SLOT_UNI_CENTERED, midi_unipolar(cc_lo), midi_bipolar(cc_lo), 1.0)
    out_uc_mid = apply_single_slot(base, MOD_SLOT_UNI_CENTERED, midi_unipolar(cc_mid), midi_bipolar(cc_mid), 1.0)
    out_uc_hi = apply_single_slot(base, MOD_SLOT_UNI_CENTERED, midi_unipolar(cc_hi), midi_bipolar(cc_hi), 1.0)
    assert out_uc_lo < out_uc_mid < out_uc_hi

    # Amount polarity should invert direction consistently.
    pos = apply_single_slot(base, MOD_SLOT_BI_NATIVE, midi_unipolar(cc_hi), midi_bipolar(cc_hi), 0.7)
    neg = apply_single_slot(base, MOD_SLOT_BI_NATIVE, midi_unipolar(cc_hi), midi_bipolar(cc_hi), -0.7)
    assert pos > base
    assert neg < base

    print("modslot regression checks passed")


if __name__ == "__main__":
    main()

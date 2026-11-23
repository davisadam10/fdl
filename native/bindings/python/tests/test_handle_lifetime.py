# SPDX-FileCopyrightText: 2024-present American Society Of Cinematographers
# SPDX-License-Identifier: Apache-2.0
"""Regression tests for Python handle lifetime, child/collection UAF guards, and cleanup."""

from __future__ import annotations

import gc
import warnings

import pytest
from fdl.fdl_types import DimensionsFloat, DimensionsInt, PointFloat

from fdl import (
    FDL,
    FDLValidationError,
    get_anchor_from_path,
    get_dimensions_from_path,
    read_from_string,
)


def _make_doc() -> FDL:
    doc = FDL.create(
        uuid="aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee",
        version_major=2,
        version_minor=0,
        fdl_creator="test",
        default_framing_intent="FI_01",
    )
    doc.add_framing_intent("FI_01", "Default", DimensionsInt(width=16, height=9), 0.0)
    ctx = doc.add_context("Source", "test")
    cvs = ctx.add_canvas("CV_01", "Canvas", "CV_01", DimensionsInt(width=3840, height=2160), 1.0)
    cvs.add_framing_decision(
        "CV_01-FI_01",
        "FD",
        "FI_01",
        DimensionsFloat(width=3840.0, height=2160.0),
        PointFloat(x=0.0, y=0.0),
    )
    return doc


def test_child_handle_raises_after_doc_close() -> None:
    doc = _make_doc()
    ctx = doc.contexts[0]
    cvs = ctx.canvases[0]
    fd = cvs.framing_decisions[0]
    fi = doc.framing_intents[0]

    doc.close()

    with pytest.raises(RuntimeError, match="closed"):
        _ = ctx.label
    with pytest.raises(RuntimeError, match="closed"):
        _ = cvs.id
    with pytest.raises(RuntimeError, match="closed"):
        _ = fd.id
    with pytest.raises(RuntimeError, match="closed"):
        _ = fi.id


def test_collection_wrapper_raises_after_doc_close() -> None:
    doc = _make_doc()
    contexts = doc.contexts
    canvases = contexts[0].canvases

    doc.close()

    with pytest.raises(RuntimeError, match="closed"):
        _ = len(contexts)
    with pytest.raises(RuntimeError, match="closed"):
        _ = contexts[0]
    with pytest.raises(RuntimeError, match="closed"):
        _ = contexts.get_by_id("Source")
    with pytest.raises(RuntimeError, match="closed"):
        _ = len(canvases)


def test_geometry_utils_reject_closed_handles() -> None:
    doc = _make_doc()
    cvs = doc.contexts[0].canvases[0]
    fd = cvs.framing_decisions[0]
    doc.close()

    with pytest.raises(RuntimeError, match="closed"):
        get_dimensions_from_path(cvs, fd, "framing_decision.dimensions")
    with pytest.raises(RuntimeError, match="closed"):
        get_anchor_from_path(cvs, fd, "framing_decision.dimensions")


def test_read_from_string_closes_doc_on_validation_error() -> None:
    # Missing required fields -> triggers FDLValidationError, must not emit ResourceWarning
    invalid_json = '{"uuid": "not-a-uuid"}'
    gc.collect()
    with warnings.catch_warnings(record=True) as caught:
        warnings.simplefilter("always", ResourceWarning)
        with pytest.raises(FDLValidationError):
            read_from_string(invalid_json, validate=True)
        gc.collect()
    resource_warnings = [w for w in caught if issubclass(w.category, ResourceWarning)]
    assert resource_warnings == []
